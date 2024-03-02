#include <windows.h>
#include <stdio.h>
#include <winuser.h>
#include <stdbool.h>
#include <time.h>

int SaveClipboardImageToFile(const char *filename)
{
    if (!OpenClipboard(NULL))
    {
        fprintf(stderr, "Failed to open clipboard.\n");
        return 1;
    }

    HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (hBitmap == NULL)
    {
        fprintf(stderr, "No bitmap found on clipboard.\n");
        CloseClipboard();
        return 1;
    }

    BITMAP bitmap;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bitmap))
    {
        fprintf(stderr, "Failed to get object information.\n");
        CloseClipboard();
        return 1;
    }

    // Determine the color format
    int colorDepth = bitmap.bmBitsPixel;
    int bytesPerPixel = colorDepth / 8;

    // Adjust the color format to ensure proper rendering
    int biBitCount = (bytesPerPixel == 3) ? 24 : 32;

    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file for writing.\n");
        CloseClipboard();
        return 1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = bitmap.bmWidth;
    infoHeader.biHeight = bitmap.bmHeight;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = biBitCount; // Adjusted color format
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    fileHeader.bfType = 0x4D42; // 'BM'
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bitmap.bmWidthBytes * bitmap.bmHeight;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    // Get the bitmap bits and write them to the file
    BYTE *bits = (BYTE *)malloc(bitmap.bmWidthBytes * bitmap.bmHeight);
    if (!bits)
    {
        fclose(file);
        CloseClipboard();
        return 1;
    }

    if (!GetBitmapBits(hBitmap, bitmap.bmWidthBytes * bitmap.bmHeight, bits))
    {
        free(bits);
        fclose(file);
        CloseClipboard();
        return 1;
    }

    // Flip the image upside down
    BYTE *flippedBits = (BYTE *)malloc(bitmap.bmWidthBytes * bitmap.bmHeight);
    for (int y = 0; y < bitmap.bmHeight; ++y)
    {
        memcpy(flippedBits + (bitmap.bmHeight - y - 1) * bitmap.bmWidthBytes, bits + y * bitmap.bmWidthBytes, bitmap.bmWidthBytes);
    }

    fwrite(flippedBits, bitmap.bmWidthBytes * bitmap.bmHeight, 1, file);

    free(bits);
    fclose(file);
    CloseClipboard();

    printf("Image saved to %s\n", filename);

    return 0;
}

int main()
{
    DWORD last = GetClipboardSequenceNumber();

    while (true)
    {
        Sleep(500);                                // Sleep for 0.5 second
        DWORD curr = GetClipboardSequenceNumber(); // Get last 'copy' ID
        if (curr != last)                          // Check if ID changed aka new copy action
        {
            char filename[MAX_PATH];
            time_t rawtime;
            struct tm *timeinfo;

            // Get the current system time in seconds since the epoch and store it in 'rawtime'
            time(&rawtime);

            // Convert the raw time data into a local time structure and store it in 'timeinfo'
            timeinfo = localtime(&rawtime);

            // Format the current time using the specified format and store it in 'filename'
            strftime(filename, MAX_PATH, "C:\\tmp\\clipboard_image_%Y%m%d_%H%M%S.bmp", timeinfo);

            SaveClipboardImageToFile(filename);
            last = curr; // Update last after processing the clipboard change
        }
    }

    return 0;
}