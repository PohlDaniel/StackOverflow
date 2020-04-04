#pragma comment (lib, "olepro32.lib")   // for IPicture COM interface support

#include <iostream>
#include <olectl.h.>    // for OleLoadPicture() and IPicture COM interface
#include <vector>
#include "Bitmap.h"

Bitmap::Bitmap(){

	Bitmap::data = NULL;
	Bitmap::dataMatrix = NULL;
	Bitmap::bpp = 3;
}

 
Bitmap::~Bitmap()
{
	if(Bitmap::data){
		free(Bitmap::data);
		Bitmap::data = NULL;
	}

	if(Bitmap::dataMatrix){
		for(int j=0; j<Bitmap::height; j++  )
		{
		free(Bitmap::dataMatrix[j]);
		Bitmap::dataMatrix[j]=NULL;
		}
		free(Bitmap::dataMatrix);
		Bitmap::dataMatrix=NULL;
	}
}


void Bitmap::selectObject()
{
	if (dc)
		m_hPrevObj = SelectObject(dc, hBitmap);
}

void Bitmap::deselectObject()
{
	if (dc && m_hPrevObj)
	{
		SelectObject(dc, m_hPrevObj);
		m_hPrevObj = 0;
	}
}

void Bitmap::destroy(){
	deselectObject();

	if (hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = 0;
	}

	if (dc)
	{
		DeleteDC(dc);
		dc = 0;
	}

	width = height = padWidth = 0;
	m_hPrevObj = 0;
	data = 0;
}

bool Bitmap::create(int widthPixels, int heightPixels){
	destroy();

	width = widthPixels;
	height = heightPixels;
	padWidth = ((width * 32 + 31) & ~31) >> 3;
	dc = CreateCompatibleDC(0);

	if (!dc)
		return false;

	memset(&info, 0, sizeof(info));

	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biPlanes = 1;

	hBitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&data), 0, 0);

	if (!hBitmap){
		destroy();
		return false;
	}

	GdiFlush();
	return true;
}

bool Bitmap::loadPicture(LPCTSTR pszFilename){

	// Loads an image using the IPicture COM interface.
	// Supported image formats: BMP, EMF, GIF, ICO, JPG, WMF, TGA.
	//
	// Based on code from MSDN Magazine, October 2001.
	// http://msdn.microsoft.com/msdnmag/issues/01/10/c/default.aspx

	HRESULT hr = 0;
	HANDLE hFile = 0;
	HGLOBAL hGlobal = 0;
	IStream *pIStream = 0;
	IPicture *pIPicture = 0;
	BYTE *pBuffer = 0;
	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;
	LONG lWidth = 0;
	LONG lHeight = 0;

	if (!m_logpixelsx && !m_logpixelsy){

		HDC hScreenDC = CreateCompatibleDC(GetDC(0));

		if (!hScreenDC)
			return false;

		m_logpixelsx = GetDeviceCaps(hScreenDC, LOGPIXELSX);
		m_logpixelsy = GetDeviceCaps(hScreenDC, LOGPIXELSY);
		DeleteDC(hScreenDC);
	}

	hFile = CreateFile(pszFilename, FILE_READ_DATA, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	if (!(dwFileSize = GetFileSize(hFile, 0))){

		CloseHandle(hFile);
		return false;
	}

	if (!(hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, dwFileSize))){

		CloseHandle(hFile);
		return false;
	}

	if (!(pBuffer = reinterpret_cast<BYTE*>(GlobalLock(hGlobal)))){

		GlobalFree(hGlobal);
		CloseHandle(hFile);
		return false;
	}

	if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, 0)){

		GlobalUnlock(hGlobal);
		GlobalFree(hGlobal);
		CloseHandle(hFile);
		return false;
	}

	GlobalUnlock(hGlobal);
	CloseHandle(hFile);

	if (FAILED(CreateStreamOnHGlobal(hGlobal, FALSE, &pIStream))){

		GlobalFree(hGlobal);
		return false;
	}

	if (FAILED(OleLoadPicture(pIStream, 0, FALSE, IID_IPicture,
		reinterpret_cast<LPVOID*>(&pIPicture)))){

		pIStream->Release();
		GlobalFree(hGlobal);
		return false;
	}

	pIStream->Release();
	GlobalFree(hGlobal);

	pIPicture->get_Width(&lWidth);
	pIPicture->get_Height(&lHeight);

	static const int HIMETRIC_INCH = 2540;

	width = MulDiv(lWidth, m_logpixelsx, HIMETRIC_INCH);
	height = MulDiv(lHeight, m_logpixelsy, HIMETRIC_INCH);

	if (!create(width, height)){

		pIPicture->Release();
		return false;
	}

	selectObject();
	hr = pIPicture->Render(dc, 0, 0, width, height, 0, lHeight, lWidth, -lHeight, 0);
	deselectObject();

	pIPicture->Release();
	return (SUCCEEDED(hr)) ? true : false;
}

void Bitmap::flipVertical(){
	BYTE *pFront = 0;
	BYTE *pBack = 0;

	if (Bitmap::bpp == 3){


		BYTE pixel[3] = { 0 };

		for (int i = 0; i < height; ++i){

			pFront = &data[i * padWidth];
			pBack = &pFront[padWidth - 3];

			while (pFront < pBack){

				// Save current pixel at position pFront.
				pixel[0] = pFront[0];
				pixel[1] = pFront[1];
				pixel[2] = pFront[2];


				// Copy new pixel from position pBack into pFront.
				pFront[0] = pBack[0];
				pFront[1] = pBack[1];
				pFront[2] = pBack[2];


				// Copy old pixel at position pFront into pBack.
				pBack[0] = pixel[0];
				pBack[1] = pixel[1];
				pBack[2] = pixel[2];

				pFront += 3;
				pBack -= 3;
			}
		}
	}else if (Bitmap::bpp == 4){

		BYTE pixel[4] = { 0 };

		for (int i = 0; i < height; ++i){

			pFront = &data[i * padWidth];
			pBack = &pFront[padWidth - 4];

			while (pFront < pBack){

				// Save current pixel at position pFront.
				pixel[0] = pFront[0];
				pixel[1] = pFront[1];
				pixel[2] = pFront[2];
				pixel[3] = pFront[3];

				// Copy new pixel from position pBack into pFront.
				pFront[0] = pBack[0];
				pFront[1] = pBack[1];
				pFront[2] = pBack[2];
				pFront[3] = pBack[3];

				// Copy old pixel at position pFront into pBack.
				pBack[0] = pixel[0];
				pBack[1] = pixel[1];
				pBack[2] = pixel[2];
				pBack[3] = pixel[3];

				pFront += 4;
				pBack -= 4;
			}
		}
	}
}

void Bitmap::flipHorizontal(){
	std::vector<BYTE> srcPixels(padWidth * height);

	memcpy(&srcPixels[0], data, padWidth * height);

	BYTE *pSrcRow = 0;
	BYTE *pDestRow = 0;

	for (int i = 0; i < height; ++i){

		pSrcRow = &srcPixels[(height - 1 - i) * padWidth];
		pDestRow = &data[i * padWidth];
		memcpy(pDestRow, pSrcRow, padWidth);
	}
}



// LoadBitmapFile
// desc: Returns a pointer to the bitmap image of the bitmap specified
//       by filename. Also returns the bitmap header information.
//		 No support for 8-bit bitmaps.
bool Bitmap::loadBitmap24(const char *filename){
	
	FILE 				*filePtr;				// the file pointer
	unsigned char		tempRGB;				// swap variable
	//int					padWidth;				// widht of a padded row
	int					paddingByte;		  // number of Bytes to fill up the row to a multiple of four

	// open filename in "read binary" mode
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return false;

	// read the bitmap file header
	fread(&(Bitmap::bmfh), sizeof(BITMAPFILEHEADER), 1, filePtr);
	
	// verify that this is a bitmap by checking for the universal bitmap id
	if (Bitmap::bmfh.bfType != 0x4D42){
		fclose(filePtr);
		return false;
	}

	// read the bitmap information header
	fread(&(Bitmap::bmih), sizeof(BITMAPINFOHEADER), 1, filePtr);
	Bitmap::width = Bitmap::bmih.biWidth;
	Bitmap::height = Bitmap::bmih.biHeight;

	padWidth = 3*Bitmap::width;
	 while(padWidth%4!=0) {
        padWidth++;
    }

	paddingByte =padWidth-3*Bitmap::width;

	// move file pointer to beginning of bitmap data
	fseek(filePtr, Bitmap::bmfh.bfOffBits, SEEK_SET);

	// allocate enough memory for the bitmap image data
	Bitmap::data = (unsigned char*)malloc(bmih.biSizeImage);

	// verify memory allocation
	if (!Bitmap::data){
		free(Bitmap::data);
		fclose(filePtr);
		return false;
	}

	// read in the bitmap image data
	fread(Bitmap::data, 1, (Bitmap::bmih).biSizeImage, filePtr);

	// make sure bitmap image data was read
	if (Bitmap::data == NULL){
		fclose(filePtr);
		return false;
	}

	// swap the R and B values to get RGB since the bitmap color format is in BGR
	for (int i = 0, k = 0; i < (width * 3 + paddingByte) * height; i += 3, k += 3) {

		tempRGB = data[i];
		data[i] = data[i + 2];
		data[i + 2] = tempRGB;

		//jump over the padding at the end of a line
		if (((k + 3) + paddingByte) % padWidth == 0) {
			i += paddingByte;
			k = -3;
		}
	}
	// close the file and return the bitmap image data
	fclose(filePtr);

	Bitmap::bpp = 3;

	return true;
}

bool Bitmap::loadBitmap24toRGBA(const char *filename){

	FILE 				*filePtr;				// the file pointer
	unsigned char		tempRGB;				// swap variable
	int					paddingByte;			// number of Bytes to fill up the row to a multiple of four

	// open filename in "read binary" mode
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return false;

	// read the bitmap file header
	fread(&(Bitmap::bmfh), sizeof(BITMAPFILEHEADER), 1, filePtr);

	// verify that this is a bitmap by checking for the universal bitmap id
	if (Bitmap::bmfh.bfType != 0x4D42){
		fclose(filePtr);
		return false;
	}

	// read the bitmap information header
	fread(&(Bitmap::bmih), sizeof(BITMAPINFOHEADER), 1, filePtr);
	Bitmap::width = Bitmap::bmih.biWidth;
	Bitmap::height = Bitmap::bmih.biHeight;

	padWidth = 3 * Bitmap::width;
	while (padWidth % 4 != 0) {
		padWidth++;
	}

	paddingByte = padWidth - 3 * Bitmap::width;

	// move file pointer to beginning of bitmap data
	fseek(filePtr, Bitmap::bmfh.bfOffBits, SEEK_SET);

	// allocate enough memory for the bitmap image data
	unsigned char *inputData = (unsigned char*)malloc(bmih.biSizeImage);

	if (!inputData){
		free(inputData);
		fclose(filePtr);
		return false;
	}

	// read in the bitmap image data
	fread(inputData, 1, (Bitmap::bmih).biSizeImage, filePtr);

	// make sure bitmap image data was read
	if (inputData == NULL){
		fclose(filePtr);
		return false;
	}

	// swap the R and B values to get RGB since the bitmap color format is in BGR
	for (int i = 0, k = 0; i < (width * 3 + paddingByte) * height; i += 3, k +=3) {
		
		tempRGB = inputData[i];
		inputData[i] = inputData[i + 2];
		inputData[i + 2] = tempRGB;

		//jump over the padding at the end of a line
		if (((k + 3)  + paddingByte) % padWidth == 0) {
			i += paddingByte;
			k = -3;
		}
	}

	Bitmap::data = (unsigned char*)malloc(bmih.biWidth * 4 * bmih.biHeight);

	// verify memory allocation
	if (!Bitmap::data){
		free(Bitmap::data);
		fclose(filePtr);
		return false;
	}

	for (int k = 0; k < height; k++){
		for (int i = 0, j = 0; j < width * 3; i += 4, j += 3) {

			data[4 * width *k + i] = inputData[padWidth *k + j];
			data[4 * width *k + i + 1] = inputData[padWidth *k + j + 1];
			data[4 * width *k + i + 2] = inputData[padWidth *k + j + 2];
			data[4 * width *k + i + 3] = (unsigned char)255;
		}
	}

	Bitmap::bpp = 4;
	padWidth = 4 * Bitmap::width;
	free(inputData);
	return true;
}

bool Bitmap::readMonochrome(const char *filename)
{
    FILE *filePtr = fopen(filename,"rb");

    // BMP header is 54 bytes
   	fread(&(Bitmap::bmfh), sizeof(BITMAPFILEHEADER), 1, filePtr);

	// read the bitmap information header
	fread(&(Bitmap::bmih), sizeof(BITMAPINFOHEADER), 1, filePtr);
	
	Bitmap::width=bmih.biWidth;
	Bitmap::height=bmih.biHeight;

	int w = Bitmap::bmih.biWidth;
	int h = Bitmap::bmih.biHeight;

	

    // lines are aligned on 4-byte boundary
    int lineSize = (w / 32) * 4;
		 if( w % 32 )lineSize += 4;
    int fileSize = lineSize * h;

	

	Bitmap::data = (unsigned char *)malloc(w * h);
	unsigned char *tempData =(unsigned char *) malloc(fileSize);

    // skip the header
    fseek(filePtr,54,SEEK_SET);

    // skip palette - two rgb quads, 8 bytes
    fseek(filePtr, 8, SEEK_CUR);

    // read data
    fread(tempData,1,fileSize,filePtr);

    // decode bits
    int i, j, k, rev_j;
    for(j = 0, rev_j = h - 1; j < h ; j++, rev_j--) {
        for(i = 0 ; i < w / 8; i++) {
            int fpos = j * lineSize + i, pos = rev_j * w + i * 8;
            for(k = 0 ; k < 8 ; k++)
				Bitmap::data[pos + (7 - k)] = (tempData[fpos] >> k ) & 1;
			
        }
    }

		

    free(tempData);
    
    return true;
}


bool Bitmap::loadBitmap24B(const char *filename)
{
	FILE *filePtr;								// the file pointer
	//int					padWidth;				// widht of a padded row
	int					paddingByte;			// number of Bytes to fill up the row to a multiple of four

	// open filename in "read binary" mode
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return false;

	// read the bitmap file header
	fread(&(Bitmap::bmfh), sizeof(BITMAPFILEHEADER), 1, filePtr);
	
	// verify that this is a bitmap by checking for the universal bitmap id
	if (Bitmap::bmfh.bfType != 0x4D42)
	{
		fclose(filePtr);
		return false;
	}

	// read the bitmap information header
	fread(&(Bitmap::bmih), sizeof(BITMAPINFOHEADER), 1, filePtr);
	Bitmap::width = Bitmap::bmih.biWidth;
	Bitmap::height = Bitmap::bmih.biHeight;
	
	
	padWidth = 3*Bitmap::width;
	 while(padWidth%4!=0) {
        padWidth++;
    }

	paddingByte=padWidth-3*Bitmap::width;

	// move file pointer to beginning of bitmap data
	fseek(filePtr, Bitmap::bmfh.bfOffBits, SEEK_SET);

	// allocate enough memory for the bitmap image data
	Bitmap::data = (unsigned char*)malloc(bmih.biSizeImage);
	
	// verify memory allocation
	if (!Bitmap::data)
	{
		free(Bitmap::data);
		fclose(filePtr);
		return false;
	}

	// read in the bitmap image data
	fread(Bitmap::data, 1, (Bitmap::bmih).biSizeImage, filePtr);

	// make sure bitmap image data was read
	if (Bitmap::data == NULL)
	{
		fclose(filePtr);
		return false;
	}
	

	// allocate enough memory for the B-color data
	Bitmap::dataMatrix = (unsigned char**)malloc(Bitmap::height* sizeof(unsigned char*) );

	for(int j=0; j<Bitmap::height; j++  )
	{
	   dataMatrix[j] = (unsigned char *)malloc(Bitmap::width* sizeof(unsigned char) );
	}

		
	// storing the B-color data inside the dataMatrix
	for(int j=0;j<Bitmap::height;j++) {

		for(int i=0;i<Bitmap::width;i++) {

			dataMatrix[j][i] = data[j*padWidth + 3*i];
		}
	}

	// close the file and return the bitmap image data
	fclose(filePtr);

	return true;
}