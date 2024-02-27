#include <windows.h>
#include <stdio.h>

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
    if (SaveClipboardImageToFile("C:\\tmp\\clipboard_image1.bmp") != 0)
    {
        fprintf(stderr, "Failed to save clipboard image to file.\n");
        return 1;
    }

    return 0;
}