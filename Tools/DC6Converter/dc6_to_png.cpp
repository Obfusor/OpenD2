// dc6_to_png - Standalone DC6 to PNG converter for OpenD2
//
// Usage: dc6_to_png <data_directory>
//
// Recursively finds all .dc6 files under <data_directory>,
// decodes each frame using Act 1 palette, and saves as PNG.
//
// Output: For each foo.dc6, creates foo/ directory containing:
//   0.png, 1.png, ... (single direction)
//   d0f0.png, d0f1.png, ... (multi direction)
//   meta.txt (frame dimensions and offsets)
//
// Requires Allegro 5 (for PNG writing via allegro_image).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#ifndef ALLEGRO_NO_MAGIC_MAIN
#define ALLEGRO_NO_MAGIC_MAIN
#endif
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

// DC6 structures (matching Engine/DC6.hpp)
#pragma pack(push, 1)
struct DC6FrameHeader
{
	uint32_t dwFlip;
	uint32_t dwWidth;
	uint32_t dwHeight;
	uint32_t dwOffsetX;
	uint32_t dwOffsetY;
	uint32_t dwUnknown;
	uint32_t dwNextBlock;
	uint32_t dwLength;
};

struct DC6ImageHeader
{
	uint32_t dwVersion;
	uint32_t dwUnk1;
	uint32_t dwUnk2;
	uint32_t dwTermination;
	uint32_t dwDirections;
	uint32_t dwFrames;
};
#pragma pack(pop)

// Palette: 256 entries of {R, G, B}
static uint8_t g_palette[256][3];

// Recursively find all .dc6 files
static void FindDC6Files(const std::string& dir, std::vector<std::string>& results)
{
#ifdef _WIN32
	std::string pattern = dir + "\\*";
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (fd.cFileName[0] == '.')
			continue;

		std::string fullPath = dir + "\\" + fd.cFileName;

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			FindDC6Files(fullPath, results);
		}
		else
		{
			size_t len = strlen(fd.cFileName);
			if (len > 4)
			{
				const char* ext = fd.cFileName + len - 4;
				if (_stricmp(ext, ".dc6") == 0)
					results.push_back(fullPath);
			}
		}
	} while (FindNextFileA(hFind, &fd));
	FindClose(hFind);
#else
	DIR* d = opendir(dir.c_str());
	if (!d) return;

	struct dirent* entry;
	while ((entry = readdir(d)) != nullptr)
	{
		if (entry->d_name[0] == '.')
			continue;

		std::string fullPath = dir + "/" + entry->d_name;
		struct stat st;
		if (stat(fullPath.c_str(), &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode))
			FindDC6Files(fullPath, results);
		else
		{
			size_t len = strlen(entry->d_name);
			if (len > 4)
			{
				const char* ext = entry->d_name + len - 4;
				if (strcasecmp(ext, ".dc6") == 0)
					results.push_back(fullPath);
			}
		}
	}
	closedir(d);
#endif
}

static void MakeDirs(const char* path)
{
	char tmp[2048];
	snprintf(tmp, sizeof(tmp), "%s", path);
	for (char* p = tmp + 1; *p; p++)
	{
		if (*p == '\\' || *p == '/')
		{
			*p = '\0';
			MKDIR(tmp);
			*p = '\\';
		}
	}
	MKDIR(tmp);
}

static bool LoadPaletteFromFile(const char* palPath)
{
	FILE* fp = fopen(palPath, "rb");
	if (fp)
	{
		size_t read = fread(g_palette, 1, 768, fp);
		fclose(fp);
		if (read == 768)
		{
			printf("Loaded palette from: %s\n", palPath);
			return true;
		}
	}
	return false;
}

static bool LoadPalette(const char* dataDir)
{
	// Try Act 1 palette first, then units palette
	const char* palPaths[] = {
		"global\\palette\\ACT1\\pal.dat",
		"global\\palette\\units\\pal.dat",
	};

	for (int i = 0; i < 2; i++)
	{
		char fullPath[2048];
		snprintf(fullPath, sizeof(fullPath), "%s\\%s", dataDir, palPaths[i]);
		if (LoadPaletteFromFile(fullPath))
			return true;
	}

	printf("Warning: No palette found in %s. Using grayscale fallback.\n", dataDir);
	for (int i = 0; i < 256; i++)
	{
		g_palette[i][0] = (uint8_t)i;
		g_palette[i][1] = (uint8_t)i;
		g_palette[i][2] = (uint8_t)i;
	}
	return false;
}

// Walk up from a file path looking for the data/ directory to find palettes
static bool LoadPaletteFromNearbyData(const char* dc6Path)
{
	char search[2048];
	snprintf(search, sizeof(search), "%s", dc6Path);

	// Walk up directories looking for data\global\palette\ACT1\pal.dat
	for (char* p = search + strlen(search); p > search; p--)
	{
		if (*p == '\\' || *p == '/')
		{
			*p = '\0';
			char palPath[2048];
			snprintf(palPath, sizeof(palPath), "%s\\global\\palette\\ACT1\\pal.dat", search);
			if (LoadPaletteFromFile(palPath))
				return true;
			// Also check if we're already inside data/
			snprintf(palPath, sizeof(palPath), "%s\\palette\\ACT1\\pal.dat", search);
			if (LoadPaletteFromFile(palPath))
				return true;
		}
	}
	return false;
}

static int ConvertOneDC6(const char* dc6Path)
{
	FILE* fp = fopen(dc6Path, "rb");
	if (!fp)
		return -1;

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fileSize <= (long)sizeof(DC6ImageHeader) || fileSize > 8 * 1024 * 1024)
	{
		fclose(fp);
		return -1;
	}

	uint8_t* fileData = (uint8_t*)malloc(fileSize);
	if (!fileData) { fclose(fp); return -1; }
	fread(fileData, 1, fileSize, fp);
	fclose(fp);

	DC6ImageHeader* header = (DC6ImageHeader*)fileData;
	if (header->dwVersion != 6)
	{
		free(fileData);
		return -1;
	}

	uint32_t numDirs = header->dwDirections;
	uint32_t framesPerDir = header->dwFrames;
	uint32_t totalFrames = numDirs * framesPerDir;

	if (totalFrames == 0 || totalFrames > 10000)
	{
		free(fileData);
		return -1;
	}

	uint32_t* framePointers = (uint32_t*)(fileData + sizeof(DC6ImageHeader));

	// Output directory: strip .dc6
	char outDir[2048];
	snprintf(outDir, sizeof(outDir), "%s", dc6Path);
	size_t pathLen = strlen(outDir);
	if (pathLen > 4)
		outDir[pathLen - 4] = '\0';

	// Check if already converted (last frame PNG exists)
	char checkPath[2048];
	if (numDirs == 1)
		snprintf(checkPath, sizeof(checkPath), "%s\\%u.png", outDir, framesPerDir - 1);
	else
		snprintf(checkPath, sizeof(checkPath), "%s\\d%uf%u.png", outDir, numDirs - 1, framesPerDir - 1);

	FILE* checkFp = fopen(checkPath, "rb");
	if (checkFp)
	{
		fclose(checkFp);
		free(fileData);
		return 0; // already converted
	}

	MakeDirs(outDir);

	static uint8_t decodeBuffer[2048 * 2048];
	int framesConverted = 0;

	for (uint32_t d = 0; d < numDirs; d++)
	{
		for (uint32_t f = 0; f < framesPerDir; f++)
		{
			uint32_t frameIdx = d * framesPerDir + f;
			uint32_t frameOffset = framePointers[frameIdx];

			if (frameOffset + sizeof(DC6FrameHeader) > (uint32_t)fileSize)
				continue;

			DC6FrameHeader* fh = (DC6FrameHeader*)(fileData + frameOffset);
			uint32_t w = fh->dwWidth;
			uint32_t h = fh->dwHeight;

			if (w == 0 || h == 0 || w > 2048 || h > 2048)
				continue;

			// Decode RLE to palette indices
			memset(decodeBuffer, 0, w * h);
			uint8_t* encoded = fileData + frameOffset + sizeof(DC6FrameHeader);
			uint32_t encodedLen = fh->dwLength;

			uint32_t x = 0;
			uint32_t y = (fh->dwFlip > 0) ? 0 : h - 1;

			for (uint32_t i = 0; i < encodedLen; i++)
			{
				uint8_t pixel = encoded[i];
				if (pixel == 0x80)
				{
					x = 0;
					if (fh->dwFlip > 0) y++;
					else y--;
				}
				else if (pixel & 0x80)
				{
					x += pixel & 0x7F;
				}
				else
				{
					uint8_t count = pixel;
					while (count--)
					{
						i++;
						if (y < h && x < w && i < encodedLen)
							decodeBuffer[y * w + x] = encoded[i];
						x++;
					}
				}
			}

			// Create Allegro bitmap and fill with RGBA pixels
			ALLEGRO_BITMAP* bmp = al_create_bitmap(w, h);
			if (!bmp)
				continue;

			ALLEGRO_LOCKED_REGION* lr = al_lock_bitmap(bmp,
				ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
			if (lr)
			{
				for (uint32_t py = 0; py < h; py++)
				{
					uint32_t* dst = (uint32_t*)((char*)lr->data + py * lr->pitch);
					for (uint32_t px = 0; px < w; px++)
					{
						uint8_t idx = decodeBuffer[py * w + px];
						if (idx == 0)
						{
							dst[px] = 0x00000000; // transparent
						}
						else
						{
							uint8_t b = g_palette[idx][0]; // pal.dat stores BGR
							uint8_t g = g_palette[idx][1];
							uint8_t r = g_palette[idx][2];
							dst[px] = 0xFF000000 | (b << 16) | (g << 8) | r; // ABGR
						}
					}
				}
				al_unlock_bitmap(bmp);
			}

			// Save PNG
			char pngPath[2048];
			if (numDirs == 1)
				snprintf(pngPath, sizeof(pngPath), "%s\\%u.png", outDir, f);
			else
				snprintf(pngPath, sizeof(pngPath), "%s\\d%uf%u.png", outDir, d, f);

			al_save_bitmap(pngPath, bmp);
			al_destroy_bitmap(bmp);
			framesConverted++;
		}
	}

	// Write metadata
	char metaPath[2048];
	snprintf(metaPath, sizeof(metaPath), "%s\\meta.txt", outDir);
	FILE* metaFp = fopen(metaPath, "w");
	if (metaFp)
	{
		fprintf(metaFp, "directions %u\n", numDirs);
		fprintf(metaFp, "framesPerDir %u\n", framesPerDir);

		for (uint32_t d = 0; d < numDirs; d++)
		{
			for (uint32_t f = 0; f < framesPerDir; f++)
			{
				uint32_t frameIdx = d * framesPerDir + f;
				uint32_t frameOffset = framePointers[frameIdx];
				DC6FrameHeader* fh = (DC6FrameHeader*)(fileData + frameOffset);
				fprintf(metaFp, "frame %u %u %u %u %d %d\n",
					d, f, fh->dwWidth, fh->dwHeight,
					(int)fh->dwOffsetX, (int)fh->dwOffsetY);
			}
		}
		fclose(metaFp);
	}

	free(fileData);
	return framesConverted;
}

// Check if path ends with .dc6 (case-insensitive)
static bool IsDC6File(const char* path)
{
	size_t len = strlen(path);
	if (len < 5) return false;
	const char* ext = path + len - 4;
#ifdef _WIN32
	return _stricmp(ext, ".dc6") == 0;
#else
	return strcasecmp(ext, ".dc6") == 0;
#endif
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("dc6_to_png - Convert Diablo II DC6 sprites to PNG\n");
		printf("\n");
		printf("Usage:\n");
		printf("  dc6_to_png <file.dc6> [--palette <pal.dat>]   Convert one file\n");
		printf("  dc6_to_png <directory> [--palette <pal.dat>]   Convert all .dc6 files\n");
		printf("\n");
		printf("Output is placed next to each .dc6 in a same-name folder:\n");
		printf("  foo.dc6 -> foo/0.png, foo/1.png, ..., foo/meta.txt\n");
		printf("\n");
		printf("If --palette is not given, searches for data\\global\\palette\\ACT1\\pal.dat\n");
		printf("relative to the input path.\n");
		return 1;
	}

	const char* target = argv[1];
	const char* paletteOverride = nullptr;

	// Parse optional --palette flag
	for (int i = 2; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "--palette") == 0)
		{
			paletteOverride = argv[i + 1];
			break;
		}
	}

	// Initialize Allegro (needed for bitmap creation and PNG saving)
	if (!al_init())
	{
		printf("Error: Failed to initialize Allegro 5\n");
		return 1;
	}
	if (!al_init_image_addon())
	{
		printf("Error: Failed to initialize Allegro image addon\n");
		return 1;
	}

	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

	// Single file mode
	if (IsDC6File(target))
	{
		if (paletteOverride)
			LoadPaletteFromFile(paletteOverride);
		else
			LoadPaletteFromNearbyData(target);

		printf("Converting: %s\n", target);
		int result = ConvertOneDC6(target);
		if (result > 0)
			printf("Done. %d frames saved.\n", result);
		else if (result == 0)
			printf("Skipped (already converted).\n");
		else
			printf("FAILED.\n");

		return (result >= 0) ? 0 : 1;
	}

	// Directory mode: convert all .dc6 files recursively
	if (paletteOverride)
		LoadPaletteFromFile(paletteOverride);
	else
		LoadPalette(target);

	printf("Scanning %s for .dc6 files...\n", target);
	std::vector<std::string> dc6Files;
	FindDC6Files(target, dc6Files);
	printf("Found %d .dc6 files\n", (int)dc6Files.size());

	int converted = 0, skipped = 0, failed = 0;

	for (size_t i = 0; i < dc6Files.size(); i++)
	{
		int result = ConvertOneDC6(dc6Files[i].c_str());
		if (result > 0)
		{
			converted++;
			printf("[%d/%d] Converted: %s (%d frames)\n",
				(int)(i + 1), (int)dc6Files.size(),
				dc6Files[i].c_str(), result);
		}
		else if (result == 0)
		{
			skipped++;
		}
		else
		{
			failed++;
			printf("[%d/%d] FAILED: %s\n",
				(int)(i + 1), (int)dc6Files.size(),
				dc6Files[i].c_str());
		}
	}

	printf("\nDone. Converted: %d, Skipped (already done): %d, Failed: %d\n",
		converted, skipped, failed);

	return 0;
}
