#include "GeometricTransformer.h"


PixelInterpolate::PixelInterpolate()
{
}

PixelInterpolate::~PixelInterpolate()
{
}

//Các hàm của lớp BilinearInterpolate

void BilinearInterpolate::Interpolate(float tx, float ty, uchar * pSrc, int srcWidthStep, int nChannels, uchar * pDst)
{
	//toạ độ 
	int x1, y1, x2, y2;
	x1 = floor(tx);
	y1 = floor(ty);
	x2 = x1 + 1;
	y2 = y1 + 1;

	// p11(x1,y1)			p12(x1,y2)
	//		p(tx,ty)
	// p21(x2,y1)			P22(x2,y2)
	uchar* p11 = (pSrc + x1 * srcWidthStep + y1 * nChannels);
	uchar* p21 = (pSrc + x2 * srcWidthStep + y1 * nChannels);
	uchar* p12 = (pSrc + x1 * srcWidthStep + y2 * nChannels);
	uchar* p22 = (pSrc + x2 * srcWidthStep + y2 * nChannels);

	float a = tx - x1;
	float b = ty - y1;
	for (int i = 0; i < nChannels; i++)
		pDst[i] = saturate_cast<uchar>((1 - a)*(1 - b)*p11[i] + a * (1 - b)*p21[i] + b * (1 - a)*p12[i] + a * b*p22[i]);;
}

BilinearInterpolate::BilinearInterpolate()
{
}

BilinearInterpolate::~BilinearInterpolate()
{
}

//Các hàm của lớp NearestNeighborInterpolate

void NearestNeighborInterpolate::Interpolate(float tx, float ty, uchar * pSrc, int srcWidthStep, int nChannels, uchar * pDst)
{
	int x = (int)tx, y = (int)ty;
	uchar * p = pSrc + (x * srcWidthStep + y * nChannels);

	for (int i = 0; i < nChannels; i++)
		pDst[i] = p[i];
}

NearestNeighborInterpolate::NearestNeighborInterpolate()
{
}

NearestNeighborInterpolate::~NearestNeighborInterpolate()
{
}

//Các hàm của lớp AffineTransform

void AffineTransform::Translate(float dx, float dy)
{
	float matrix[3][3] = { { 1, 0 , dx },
	{ 0, 1, dy },
	{ 0, 0, 1 } };
	this->_matrixTransform = _matrixTransform * Mat(3, 3, CV_32F, matrix);
}

void AffineTransform::Rotate(float angle)
{
	angle = angle * M_PI / 180.0;
	float matrix[3][3] = { { cosf(angle), -sinf(angle) , 0 },
	{ sinf(angle), cosf(angle), 0 },
	{ 0, 0, 1 } };
	this->_matrixTransform = _matrixTransform * Mat(3, 3, CV_32FC1, matrix);
}

void AffineTransform::Scale(float sx, float sy)
{
	
	float matrix[3][3] = { { sx, 0 , 0 },
	{ 0, sy, 0 },
	{ 0, 0, 1 } };
	this->_matrixTransform = _matrixTransform * Mat(3, 3, CV_32FC1, matrix);
}

void AffineTransform::TransformPoint(float & x, float & y)
{

	float tempX = this->_matrixTransform.at<float>(0, 0)*x + this->_matrixTransform.at<float>(0, 1)*y + this->_matrixTransform.at<float>(0, 2);
	y = this->_matrixTransform.at<float>(1, 0)*x + this->_matrixTransform.at<float>(1, 1)*y + this->_matrixTransform.at<float>(1, 2);
	x = tempX;
}

AffineTransform::AffineTransform()
{
	_matrixTransform.create(3, 3, CV_32FC1);

	float matrix[3][3] = { { 1, 0 , 0 },
	{ 0, 1, 0 },
	{ 0, 0, 1 } };

	for (int y = 0; y < 3; y++)
		for (int x = 0; x < 3; x++)
			_matrixTransform.at<float>(x, y) = matrix[x][y];
	

}

AffineTransform::~AffineTransform()
{
}


//Các hàm của lớp GeometricTransformer

int GeometricTransformer::Transform(const Mat & beforeImage, Mat & afterImage, AffineTransform * transformer, PixelInterpolate * interpolator)
{
	if (beforeImage.empty())
		return 0;

	//w_src : chiều rộng ảnh nguồn, h_src : chiều cao ảnh nguồn
	//w_dst : chiều rộng ảnh đích, h_dst : chiều cao ảnh đích
	int w_src = beforeImage.cols, h_src = beforeImage.rows;
	int w_dst = afterImage.cols, h_dst = afterImage.rows;

	int nChannels = beforeImage.channels(); // 

	// khoang cach giữa 2 px cùng cột 2 dong liên tiếp
	int w_srcStep = beforeImage.step[0];
	int w_dstStep = afterImage.step[0];

	uchar* res = (uchar*)afterImage.data;
	uchar* src = (uchar*)beforeImage.data;
	float x, y;
	for (int i = 0; i < h_dst; i++, res += w_dstStep)
	{

		uchar* pRes = res; // con tro dong anh dich

		for (int j = 0; j < w_dst; j++, pRes += nChannels)
		{
			x = i;
			y = j;
			transformer->TransformPoint(x, y);

			if (round(x) < h_src && round(y) < w_src&& round(x) >= 0 && round(y) >= 0)
				interpolator->Interpolate(x, y, src, w_srcStep, nChannels, pRes);
		}

	}

	return 1;
}

int GeometricTransformer::RotateKeepImage(const Mat & srcImage, Mat & dstImage, float angle, PixelInterpolate * interpolator)
{
	if (srcImage.empty())
		return 0;
	
	//mainAffine dùng để tính toán ma trận cho việc xoay ảnh
	AffineTransform* mainAffine = new AffineTransform();

	//tempAffine dùng để tính toán độ dời giữa việc xoay quanh tâm và quanh gốc toạ độ
	AffineTransform* tempAffine = new AffineTransform();

	//Chuyển góc về dạng radian
	float radAngle = angle * M_PI / 180;
	int srcWidth = srcImage.cols;
	int srcHeight = srcImage.rows;

	//Tính chiều dài, chiều rộng ảnh mới
	int dstWidth = srcWidth * fabs(cosf(radAngle)) + srcHeight * fabs(sinf(radAngle));
	int dstHeight = srcWidth * fabs(sinf(radAngle)) + srcHeight * fabs(cosf(radAngle));

	//Khởi tạo ảnh mới
	dstImage.create(dstHeight, dstWidth, srcImage.type());

	//Tính toạ độ tâm của ảnh cũ
	float centerX = srcWidth / 2;
	float centerY = srcHeight / 2;

	//Thực hiện xoay tâm của ảnh cũ (quanh gốc toạ độ)
	tempAffine->Rotate(angle);
	tempAffine->TransformPoint(centerY, centerX);

	//Tính toán độ dời
	float dx= centerY - dstHeight / 2, dy= centerX - dstWidth / 2;

	//Tính toán ma trận cho việc xoay ảnh quanh tâm
	mainAffine->Rotate(-angle);
	mainAffine->Translate(dx, dy);

	Transform(srcImage, dstImage, mainAffine, interpolator);

	return 1;
}

int GeometricTransformer::RotateUnkeepImage(const Mat & srcImage, Mat & dstImage, float angle, PixelInterpolate * interpolator)
{
	if (srcImage.empty())
		return 0;

	//mainAffine dùng để tính toán ma trận cho việc xoay ảnh
	AffineTransform* mainAffine = new AffineTransform();

	//tempAffine dùng để tính toán độ dời giữa việc xoay quanh tâm và quanh gốc toạ độ
	AffineTransform* tempAffine = new AffineTransform();

	//Chuyển góc về dạng radian
	float radAngle = angle * M_PI / 180;
	int srcWidth = srcImage.cols;
	int srcHeight = srcImage.rows;

	//Tính toạ độ tâm của ảnh cũ
	float centerX = srcWidth / 2;
	float centerY = srcHeight / 2;

	//Khởi tạo ảnh mới với kích thước của ảnh cũ
	dstImage.create(srcHeight, srcWidth, srcImage.type());

	//Thực hiện xoay tâm của ảnh cũ (quanh gốc toạ độ)
	tempAffine->Rotate(angle);
	tempAffine->TransformPoint(centerY, centerX);

	//Tính toán độ dời
	float dx = centerY - srcHeight / 2, dy = centerX - srcWidth / 2;

	//Tính toán ma trận cho việc xoay ảnh quanh tâm
	mainAffine->Rotate(-angle);
	mainAffine->Translate(dx, dy);

	Transform(srcImage, dstImage, mainAffine, interpolator);

	return 1;
}

int GeometricTransformer::Scale(const Mat & srcImage, Mat & dstImage, float sx, float sy, PixelInterpolate * interpolator)
{
	if (!srcImage.data) return 0;
	AffineTransform * affineTf = new AffineTransform();
	affineTf->Scale(1.0f /sx,1.0f/ sy);

	dstImage.create(round(srcImage.rows * sx), round(srcImage.cols * sy), srcImage.type()); // khởi tạo ảnh đích
	int res = Transform(srcImage, dstImage, affineTf, interpolator);
	return res;
}

int GeometricTransformer::Resize(const Mat & srcImage, Mat & dstImage, int newWidth, int newHeight, PixelInterpolate * interpolator)
{
	if (!srcImage.data) return 0;
	AffineTransform * affineTf = new AffineTransform();
	float sx =((float)newWidth / (float)srcImage.rows)  , sy = ((float)newHeight / (float)srcImage.cols);
	affineTf->Scale(1.0f / sx , 1.0f / sy);

	dstImage.create(newWidth, newHeight, srcImage.type()); // khởi tạo ảnh đích
	int res = Transform(srcImage, dstImage, affineTf, interpolator);
	return res;
}

int GeometricTransformer::Flip(const Mat & srcImage, Mat & dstImage, bool direction, PixelInterpolate * interpolator)
{
	if (!srcImage.data) return 0;
	int row = srcImage.rows, col = srcImage.cols;
	dstImage.create(row, col, srcImage.type());
	int k;
	if (direction == 0) {
		for (int i = 0; i < row; i++)
			for (int j = 0, k = col - 1; j <= k; j++, k--) {
				dstImage.at<cv::Vec3b>(i, j) = srcImage.at<cv::Vec3b>(i, k);
				dstImage.at<cv::Vec3b>(i, k) = srcImage.at<cv::Vec3b>(i, j);
			}
	}
	else
	{
		for (int i = 0; i < col; i++)
			for (int j = 0, k = row - 1; j <= k; j++, k--) {
				dstImage.at<cv::Vec3b>(j, i) = srcImage.at<cv::Vec3b>(k, i);
				dstImage.at<cv::Vec3b>(k, i) = srcImage.at<cv::Vec3b>(j, i);
			}
	}
	return 1;
}

GeometricTransformer::GeometricTransformer()
{
}

GeometricTransformer::~GeometricTransformer()
{
}
