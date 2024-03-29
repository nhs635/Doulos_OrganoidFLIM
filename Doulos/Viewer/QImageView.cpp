

#include "QImageView.h"
#include <ipps.h>


ColorTable::ColorTable()
{
	// Color table list	
	m_cNameVector.push_back("gray");
	m_cNameVector.push_back("invgray");
	m_cNameVector.push_back("sepia");
	m_cNameVector.push_back("jet");
	m_cNameVector.push_back("parula");
	m_cNameVector.push_back("hot");
	m_cNameVector.push_back("fire");
	m_cNameVector.push_back("hsv");
	m_cNameVector.push_back("smart");
	m_cNameVector.push_back("bor");
	m_cNameVector.push_back("cool");
	m_cNameVector.push_back("gem");
	m_cNameVector.push_back("gfb");
	m_cNameVector.push_back("ice");
	m_cNameVector.push_back("lifetime2");
	m_cNameVector.push_back("vessel");
	m_cNameVector.push_back("hsv1");
	m_cNameVector.push_back("bwr");
	m_cNameVector.push_back("graysb");
	m_cNameVector.push_back("viridis");
	m_cNameVector.push_back("hsv2");
	// 새로운 파일 이름 추가 하기

	for (int i = 0; i < m_cNameVector.size(); i++)
	{
		QFile file("ColorTable/" + m_cNameVector.at(i) + ".colortable");
		file.open(QIODevice::ReadOnly);
		np::Uint8Array2 rgb(256, 3);
		file.read(reinterpret_cast<char*>(rgb.raw_ptr()), sizeof(uint8_t) * rgb.length());
		file.close();

		QVector<QRgb> temp_vector;
		for (int j = 0; j < 256; j++)
		{
			QRgb color = qRgb(rgb(j, 0), rgb(j, 1), rgb(j, 2));
			temp_vector.push_back(color);
		}
		m_colorTableVector.push_back(temp_vector);
	}
}


QImageView::QImageView(QWidget *parent) :
	QDialog(parent)
{
    // Disabled
}

QImageView::QImageView(ColorTable::colortable ctable, int width, int height, bool rgb, QWidget *parent) :
    QDialog(parent), m_bSquareConstraint(false), m_bRgbUsed(rgb)
{
    // Set default size
    resize(400, 400);
	
    // Set image size
    m_width = width;
    m_height = height;
	
    // Create layout
	m_pHBoxLayout = new QHBoxLayout(this);
    m_pHBoxLayout->setSpacing(0);
    m_pHBoxLayout->setContentsMargins(1, 1, 1, 1); // Remove bezel area

    // Create render area
    m_pRenderImage = new QRenderImage(this);
	m_pRenderImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_pHBoxLayout->addWidget(m_pRenderImage);	

    // Create QImage object  
	if (!m_bRgbUsed)
	{
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_Indexed8);
		m_pRenderImage->m_pImage->setColorCount(256);
		m_pRenderImage->m_pImage->setColorTable(m_colorTable.m_colorTableVector.at(ctable));
	}
	else
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_RGB888);

	memset(m_pRenderImage->m_pImage->bits(), 0, m_pRenderImage->m_pImage->byteCount());

	// Set layout
	setLayout(m_pHBoxLayout);

    // Initialization
    setUpdatesEnabled(true);
}

QImageView::~QImageView()
{

}

void QImageView::resizeEvent(QResizeEvent *)
{
	if (m_bSquareConstraint)
	{
		int w = this->width();
		int h = this->height();

		if (w < h)
			resize(w, w);
		else
			resize(h, h);
	}
}

void QImageView::resetSize(int width, int height, bool is_rgb)
{
	// Set image size
	m_width = width;
	m_height = height;	
	m_bRgbUsed = is_rgb;

	// Create QImage object
	QRgb rgb[256];
	if (!m_bRgbUsed)
	{
		for (int i = 0; i < 256; i++)
			rgb[i] = m_pRenderImage->m_pImage->color(i);
	}

	if (m_pRenderImage->m_pImage)
		delete m_pRenderImage->m_pImage;

	if (!m_bRgbUsed)
	{
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_Indexed8);
		m_pRenderImage->m_pImage->setColorCount(256);
		for (int i = 0; i < 256; i++)
			m_pRenderImage->m_pImage->setColor(i, rgb[i]);
	}
	else
		m_pRenderImage->m_pImage = new QImage(m_width, m_height, QImage::Format_RGB888);

	memset(m_pRenderImage->m_pImage->bits(), 0, m_pRenderImage->m_pImage->byteCount());

}

void QImageView::resetColormap(ColorTable::colortable ctable)
{
	m_pRenderImage->m_pImage->setColorTable(m_colorTable.m_colorTableVector.at(ctable));

	m_pRenderImage->update();
}

void QImageView::setHorizontalLine(int len, ...)
{
	m_pRenderImage->m_hLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pHLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setVerticalLine(int len, ...)
{
	m_pRenderImage->m_vLineLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pVLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setCircle(int len, ...)
{
	m_pRenderImage->m_circLen = len;

	va_list ap;
	va_start(ap, len);
	for (int i = 0; i < len; i++)
	{
		int n = va_arg(ap, int);
		m_pRenderImage->m_pHLineInd[i] = n;
	}
	va_end(ap);
}

void QImageView::setText(QPoint pos, const QString & str, bool is_vertical, QColor color)
{
	m_pRenderImage->m_textPos = pos;
	m_pRenderImage->m_str = str;
	m_pRenderImage->m_bVertical = is_vertical;
	m_pRenderImage->m_titleColor = !is_vertical ? color : Qt::black;
}

void QImageView::setHLineChangeCallback(const std::function<void(int)> &slot) 
{ 
	m_pRenderImage->DidChangedHLine.clear();
	m_pRenderImage->DidChangedHLine += slot; 
}

void QImageView::setVLineChangeCallback(const std::function<void(int)> &slot)
{ 
	m_pRenderImage->DidChangedVLine.clear();
	m_pRenderImage->DidChangedVLine += slot; 
}

void QImageView::setRLineChangeCallback(const std::function<void(int)> &slot)
{
	m_pRenderImage->DidChangedRLine.clear();
	m_pRenderImage->DidChangedRLine += slot;
}

void QImageView::setEnterCallback(const std::function<void(void)>& slot)
{
	m_pRenderImage->DidEnter.clear();
	m_pRenderImage->DidEnter += slot;
}

void QImageView::setLeaveCallback(const std::function<void(void)>& slot)
{
	m_pRenderImage->DidLeave.clear();
	m_pRenderImage->DidLeave += slot;
}

void QImageView::setMovedMouseCallback(const std::function<void(QPoint&)> &slot)
{
	m_pRenderImage->DidMovedMouse.clear();
	m_pRenderImage->DidMovedMouse += slot;

	m_pRenderImage->setMouseTracking(true);
}

void QImageView::setClickedMouseCallback(const std::function<void(int, int)>& slot)
{
	m_pRenderImage->DidClickedMouse.clear();
	m_pRenderImage->DidClickedMouse += slot;
}

void QImageView::setDoubleClickedMouseCallback(const std::function<void(void)> &slot)
{
	m_pRenderImage->DidDoubleClickedMouse.clear();
	m_pRenderImage->DidDoubleClickedMouse += slot;
}

void QImageView::drawImage(uint8_t* pImage)
{
	memcpy(m_pRenderImage->m_pImage->bits(), pImage, m_pRenderImage->m_pImage->byteCount());	
	m_pRenderImage->update();
}

void QImageView::drawRgbImage(uint8_t* pImage)
{		
	QImage *pImg = new QImage(pImage, m_width, m_height, QImage::Format_RGB888);
	m_pRenderImage->m_pImage = std::move(pImg);
	m_pRenderImage->update();
}




QRenderImage::QRenderImage(QWidget *parent) :
	QWidget(parent), m_pImage(nullptr), m_colorLine(0x00ff00),
	m_bMeasureDistance(false), m_nClicked(0),
	m_hLineLen(0), m_vLineLen(0), m_circLen(0), m_bRadial(false), m_bPixelPos(false),
	m_str(""), m_bVertical(false), m_titleColor(Qt::white)
{
	m_pHLineInd = new int[10];
	m_pVLineInd = new int[10];
	memset(m_pixelPos, 0, sizeof(int) * 2);
}

QRenderImage::~QRenderImage()
{
	delete m_pHLineInd;
	delete m_pVLineInd;
}

void QRenderImage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

    // Area size
    int w = this->width();
    int h = this->height();

    // Draw image
    if (m_pImage)
        painter.drawImage(QRect(0, 0, w, h), *m_pImage);

	// Draw assitive lines
	if (m_bPixelPos)
	{
		QPen pen; pen.setColor(Qt::red); pen.setWidth(1);
		painter.setPen(pen);

		QPointF p1, p2;
		p1.setX(0.0); p1.setY((double)(m_pixelPos[1] * h) / (double)m_pImage->height());
		p2.setX((double)w); p2.setY((double)(m_pixelPos[1] * h) / (double)m_pImage->height());
		painter.drawLine(p1, p2);

		p1.setY(0.0); p1.setX((double)(m_pixelPos[0] * w) / (double)m_pImage->width());
		p2.setY((double)h); p2.setX((double)(m_pixelPos[0] * w) / (double)m_pImage->width());
		painter.drawLine(p1, p2);
	}

	for (int i = 0; i < m_hLineLen; i++)
	{
		QPointF p1; p1.setX(0.0);       p1.setY((double)(m_pHLineInd[i] * h) / (double)m_pImage->height());
		QPointF p2; p2.setX((double)w); p2.setY((double)(m_pHLineInd[i] * h) / (double)m_pImage->height());

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);
	}
	for (int i = 0; i < m_vLineLen; i++)
	{
		QPointF p1, p2;
		if (!m_bRadial)
		{
			p1.setX((double)(m_pVLineInd[i] * w) / (double)m_pImage->width()); p1.setY(0.0);
			p2.setX((double)(m_pVLineInd[i] * w) / (double)m_pImage->width()); p2.setY((double)h);
		}
		else
		{			
			int circ_x = (double)(w / 2) + (double)(w / 2) * cos((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			int circ_y = (double)(h / 2) - (double)(h / 2) * sin((double)m_pVLineInd[i] / (double)m_rMax * IPP_2PI);
			p1.setX(w / 2); p1.setY(h / 2);
			p2.setX(circ_x); p2.setY(circ_y);
		}

		painter.setPen(m_colorLine);
		painter.drawLine(p1, p2);
	}
	for (int i = 0; i < m_circLen; i++)
	{
		QPointF center; center.setX(w / 2); center.setY(h / 2);
		double radius = (double)(m_pHLineInd[i] * h) / (double)m_pImage->height();

		painter.setPen(m_colorLine);
		painter.drawEllipse(center, radius, radius);
	}

	// Set text
	if (m_str != "")
	{
		QPen pen; pen.setColor(m_titleColor); pen.setWidth(1);
		painter.setPen(pen);
		QFont font; font.setBold(true); font.setPointSize(!m_bVertical ? 13 : 9);
		painter.setFont(font);

		painter.rotate(!m_bVertical ? 0 : 90);
		if (!m_bVertical)
			painter.drawText(QRect(m_textPos.x(), m_textPos.y(), w, h), Qt::AlignLeft, m_str);
		else
			painter.drawText(0, -24, this->height(), this->width(), Qt::AlignCenter, m_str);
		painter.rotate(!m_bVertical ? 0 : -90);
	}

	// Measure distance
	if (m_bMeasureDistance)
	{
		QPen pen; pen.setColor(Qt::red); pen.setWidth(3);
		painter.setPen(pen);

		QPointF p[2];
		for (int i = 0; i < m_nClicked; i++)
		{
			p[i] = QPointF(m_point[i][0], m_point[i][1]);
			painter.drawPoint(p[i]);
			
			if (i == 1)
			{
				// Connecting line
				QPen pen; pen.setColor(Qt::red); pen.setWidth(1);
				painter.setPen(pen);
				painter.drawLine(p[0], p[1]);
				
				// Euclidean distance
				double dist = sqrt((p[0].x() - p[1].x()) * (p[0].x() - p[1].x())
					+ (p[0].y() - p[1].y()) * (p[0].y() - p[1].y())) * (double)m_pImage->height() / (double)this->height();
				printf("Measured distance: %.1f\n", dist);

				QFont font; font.setBold(true);
				painter.setFont(font);
				painter.drawText((p[0] + p[1]) / 2, QString::number(dist, 'f', 1));
			}
		}
	}
}

void QRenderImage::enterEvent(QEvent *)
{
	DidEnter();
	update();
}

void QRenderImage::leaveEvent(QEvent *)
{
	DidLeave();
	update();
}

void QRenderImage::mousePressEvent(QMouseEvent *e)
{
	QPoint p = e->pos();

	if (QRect(0, 0, this->width(), this->height()).contains(p))
	{
		m_pixelPos[0] = m_bPixelPos ? (int)((double)(p.x() * m_pImage->width()) / (double)this->width()) : 0;
		m_pixelPos[1] = m_bPixelPos ? (int)((double)(p.y() * m_pImage->height()) / (double)this->height()) : 0;

		if (m_bPixelPos)
		{
			update();
			DidClickedMouse(m_pixelPos[0], m_pixelPos[1]);
		}
	}

	//if (m_hLineLen == 1)
	//{
	//	m_pHLineInd[0] = m_pImage->height() - (int)((double)(p.y() * m_pImage->height()) / (double)this->height());
	//	DidChangedHLine(m_pHLineInd[0]);
	//}
	//if (m_vLineLen == 1)
	//{
	//	if (!m_bRadial)
	//	{
	//		m_pVLineInd[0] = (int)((double)(p.x() * m_pImage->width()) / (double)this->width());
	//		DidChangedVLine(m_pVLineInd[0]);
	//	}
	//	else
	//	{
	//		double angle = atan2((double)height() / 2.0 - (double)p.y(), (double)p.x() - (double)width() / 2.0);
	//		if (angle < 0) angle += IPP_2PI;
	//		m_pVLineInd[0] = (int)(angle / IPP_2PI * m_rMax);
	//		DidChangedRLine(m_pVLineInd[0]);
	//	}
	//}

	//if (m_bMeasureDistance)
	//{		
	//	if (m_nClicked == 2) m_nClicked = 0;

	//	m_point[m_nClicked][0] = p.x(); // (int)((double)(p.x() * m_pImage->height())) / (double)this->height();
	//	m_point[m_nClicked++][1] = p.y(); // (int)((double)(p.y() * m_pImage->height())) / (double)this->height();

	//	update();
	//}
}

void QRenderImage::mouseDoubleClickEvent(QMouseEvent *)
{
	DidDoubleClickedMouse();
}

void QRenderImage::mouseMoveEvent(QMouseEvent *e)
{
	QPoint p = e->pos();

	if (QRect(0, 0, this->width(), this->height()).contains(p))
	{
		QPoint p1;
		p1.setX((int)((double)(p.x() * m_pImage->width()) / (double)this->width()));
		p1.setY((int)((double)(p.y() * m_pImage->height()) / (double)this->height()));

		DidMovedMouse(p1);
	}
}
//
//#ifdef OCT_FLIM
//if (m_pIntensity && m_pLifetime)
//{
//			QImage im(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);		
//			QImage in(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
//			QImage lf(4 * m_pIntensity->width(), m_pIntensity->height(), QImage::Format_RGB888);
//		
//			clock_t start1 = clock();
//	
//	//        // FLIM result image generation
//	
//			im.convertToFormat(QImage::Format_RGB888);
//			in.convertToFormat(QImage::Format_RGB888);
//			lf.convertToFormat(QImage::Format_RGB888);
//	//	
//	////        ippsAdd_8u_Sfs(intensity.bits(), lifetime.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);	
//	
//			ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
//			ippsAdd_8u_Sfs(in.bits(), lf.bits(), m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);
//	
//			/*m_pIntensity->convertToFormat(QImage::Format_RGB888).bits(), m_pLifetime->convertToFormat(QImage::Format_RGB888).bits(),
//	            m_pFluorescence->bits(), m_pFluorescence->byteCount(), 0);*/
//	
//			clock_t end1 = clock();
//			double elapsed = ((double)(end1 - start1)) / ((double)CLOCKS_PER_SEC);
//			printf("flim paint time : %.2f msec\n", 1000 * elapsed);
//	
//	
//	        // FLIM result image alpha adjustment
//	        ippsDiv_8u_Sfs(m_pFlimAlphaMap->bits(), m_pFluorescence->convertToFormat(QImage::Format_ARGB32_Premultiplied).bits(),
//	            m_pFlimImage->bits(), m_pFlimImage->byteCount(), 0);
//	
//	
//	         Draw FLIM result image
//	        painter.drawImage(QRect(0, 0, w, h), *m_pFlimImage);
//}
//#endif
