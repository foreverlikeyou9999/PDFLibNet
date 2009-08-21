#include "PDFPage.h"
namespace PDFLibNet
{
	PDFPage::PDFPage(AFPDFDocInterop *pdfDoc,PDFPageInterop *page)
		: _page(page)
		, _loaded(true)
		, _pdfDoc(pdfDoc)
		, _pageNumber(page->getPage())
	{
	}
	PDFPage::PDFPage(AFPDFDocInterop *pdfDoc, int page)
		: _page(0)
		, _loaded(false)
		, _pdfDoc(pdfDoc)
		, _pageNumber(page)
	{
	}


	void PDFPage::loadPage()
	{
		if(!_loaded)
		{
			PDFPageInterop *p=_pdfDoc->getPage(_pageNumber);
			if(p){
				_page=p;
				_loaded=true;
			}
		}
	}

	PDFPage::~PDFPage(void){
		if(_page && _loaded)
			delete _page;
	}

	void PDFPage::extractImages(){
		loadPage();
		_page->extractImages();
	}

	System::Drawing::Image ^PDFPage::GetImage(int index)
	{
		extractImages();
		unsigned char *bytes = _page->getImageBytes(index);
		int size = _page->getImageSize(index);		//Size in bytes
		int width = _page->getImageWidth(index);
		int height = _page->getImageHeight(index);	
		int nComp =_page->getImageNumComps(index);	//Number of bytes per pixel
		int iType = _page->getImageType(index);
		int bCount=0;
		cli::array<unsigned char> ^manbytes=nullptr;
		System::IO::MemoryStream  ^ms=nullptr;
		System::Drawing::Imaging::BitmapData ^bmpd;
		System::Drawing::Bitmap ^bmp;

		if(bytes){
			switch(iType){
				case 0 : //JPG
					manbytes=gcnew cli::array<unsigned char>(size);
					for(int i=0;i<size;++i)
						manbytes[i]=bytes[i];
					ms =gcnew System::IO::MemoryStream(manbytes);
					return System::Drawing::Image::FromStream(ms);
				case 1: //PBM
					manbytes=gcnew cli::array<unsigned char>(size);
					for(int i=0;i<size;++i)
						manbytes[i]=bytes[i];
					bmp = gcnew System::Drawing::Bitmap(width,height,System::Drawing::Imaging::PixelFormat::Format8bppIndexed);
					bmpd =bmp->LockBits(System::Drawing::Rectangle(0,0,width,height), System::Drawing::Imaging::ImageLockMode::WriteOnly, bmp->PixelFormat);
					bCount=bmpd->Stride * bmp->Height;
					//A lot faster
					Marshal::Copy(manbytes,0,bmpd->Scan0,size);

					bmp->UnlockBits(bmpd);
					return bmp;
					break; 
				case 2://PPM
					manbytes=gcnew cli::array<unsigned char>(size);
					for(int i=0;i<size;++i)
						manbytes[i]=bytes[i];

					if(nComp==4)	
						bmp = gcnew System::Drawing::Bitmap(width,height,System::Drawing::Imaging::PixelFormat::Format32bppRgb);
					else if(nComp==3)
						bmp = gcnew System::Drawing::Bitmap(width,height,System::Drawing::Imaging::PixelFormat::Format24bppRgb);
					else if(nComp==2)
						bmp = gcnew System::Drawing::Bitmap(width,height,System::Drawing::Imaging::PixelFormat::Format16bppRgb565);
					else if(nComp==1)
						bmp = gcnew System::Drawing::Bitmap(width,height,System::Drawing::Imaging::PixelFormat::Format8bppIndexed);

					bmpd =bmp->LockBits(System::Drawing::Rectangle(0,0,width,height), System::Drawing::Imaging::ImageLockMode::WriteOnly, bmp->PixelFormat);
					bCount =bmpd->Stride * bmp->Height;
					//A lot faster
					Marshal::Copy(manbytes,0,bmpd->Scan0,size);
					
					bmp->UnlockBits(bmpd);
					return bmp;

				break;
			}
			
		}
		return nullptr;

	}


	System::String ^PDFPage::ExtractText(System::Drawing::Rectangle rect)
	{
		loadPage();
		wchar_t * res = _page->extractText(rect.Left,rect.Top,rect.Right,rect.Bottom);
		return gcnew String(res);
	}
	
	xPDFStream::xPDFStream(System::IO::Stream ^stream)
		: StreamWriter(stream,System::Text::Encoding::UTF8)
		, _writeToStream(nullptr)
	{
		this->AutoFlush=true;
		if(this->_writeToStream==nullptr){		
			_writeToStream=gcnew  WriteToStreamHandler(this,&xPDFStream::_WriteToStreamFunc);
			_gchwriteToStream = GCHandle::Alloc(_gchwriteToStream);
		}
	}
	xPDFStream::xPDFStream(System::IO::Stream ^stream,System::Text::Encoding ^enc)
		: StreamWriter(stream,enc)
		, _writeToStream(nullptr)
	{
		this->AutoFlush=true;
		if(this->_writeToStream==nullptr){		
			_writeToStream=gcnew  WriteToStreamHandler(this,&xPDFStream::_WriteToStreamFunc);
			_gchwriteToStream = GCHandle::Alloc(_gchwriteToStream);
		}
	}
	xPDFStream::xPDFStream(String ^fileName)
		:StreamWriter(fileName)
	{
		this->AutoFlush=true;
		if(this->_writeToStream==nullptr){		
			_writeToStream=gcnew  WriteToStreamHandler(this,&xPDFStream::_WriteToStreamFunc);
			_gchwriteToStream = GCHandle::Alloc(_gchwriteToStream);
		}
	}

	void xPDFStream::_WriteToStreamFunc(wchar_t *str, int len)
	{
		Write(gcnew String(str));	
		
	}
	
	xPDFStream::!xPDFStream(){
		if(this->_gchwriteToStream.IsAllocated){		
			this->_gchwriteToStream.Free();
		}
	}
	xPDFStream::~xPDFStream(){
	
	}

	void *xPDFStream::GetWritePointer()
	{
		return Marshal::GetFunctionPointerForDelegate(this->_writeToStream).ToPointer();
	}
	
}