#ifndef QIMAGEVIEW_H
#define QIMAGEVIEW_H

#include <Doulos/Configuration.h>

#include <QDialog>
#include <QtCore>
#include <QtWidgets>

#include <stdarg.h>

#include <Common/array.h>
#include <Common/callback.h>
using ColorTableVector = QVector<QVector<QRgb>>;

class ColorTable
{
public:
	explicit ColorTable();

public:
	enum colortable { gray = 0, inv_gray, sepia, jet, parula, hot, fire, hsv, 
		smart, blueorange, cool, gem, greenfireblue, ice, lifetime2, vessel, hsv1, bwr, graysb, viridis, hsv2 }; // 새로 만든 colortable 이름 추가하기
	QVector<QString> m_cNameVector;
	ColorTableVector m_colorTableVector;
};
class QRenderImage;


class QImageView : public QDialog
{
	Q_OBJECT

private:
    explicit QImageView(QWidget *parent = 0); // Disabling default constructor

public:
    explicit QImageView(ColorTable::colortable ctable, int width, int height, bool rgb = false, QWidget *parent = 0);
    virtual ~QImageView();

public:
	inline QRenderImage* getRender() { return m_pRenderImage; }

protected:
    void resizeEvent(QResizeEvent *);

public:
	void resetSize(int width, int height, bool is_rgb = false);
    void resetColormap(ColorTable::colortable ctable);
	void setSquare(bool square) { m_bSquareConstraint = square; }
#ifdef OCT_FLIM
    void setRgbEnable(bool rgb) { m_bRgbUsed = rgb; }
#endif
	
public:
	void setHorizontalLine(int len, ...);
	void setVerticalLine(int len, ...);
	void setCircle(int len, ...);
	void setText(QPoint pos, const QString& str, bool is_vertical = false, QColor color = Qt::white);

	void setHLineChangeCallback(const std::function<void(int)> &slot);
	void setVLineChangeCallback(const std::function<void(int)> &slot);
	void setRLineChangeCallback(const std::function<void(int)> &slot);

public:
	void setEnterCallback(const std::function<void(void)> &slot);
	void setLeaveCallback(const std::function<void(void)> &slot);
	void setMovedMouseCallback(const std::function<void(QPoint&)> &slot);
	void setClickedMouseCallback(const std::function<void(int, int)> &slot);
	void setDoubleClickedMouseCallback(const std::function<void(void)> &slot);

public slots:
	void drawImage(uint8_t* pImage);
	void drawRgbImage(uint8_t* pImage);

private:
    QHBoxLayout *m_pHBoxLayout;

	ColorTable m_colorTable;
    QRenderImage *m_pRenderImage;

private:
    int m_width;
    int m_height;

	bool m_bSquareConstraint;
    bool m_bRgbUsed;
};


class QRenderImage : public QWidget
{
    Q_OBJECT

public:
    explicit QRenderImage(QWidget *parent = 0);
	virtual ~QRenderImage();

public:
	inline void setVisPixelPos(bool vis) { m_bPixelPos = vis; }

protected:
    void paintEvent(QPaintEvent *);
	void enterEvent(QEvent *);
	void leaveEvent(QEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseDoubleClickEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);

public:
    QImage *m_pImage;

    int *m_pHLineInd, *m_pVLineInd;
    int m_hLineLen, m_vLineLen;
    int m_circLen;
	
	bool m_bRadial;
	int m_rMax;

	bool m_bPixelPos;
	int m_pixelPos[2];

	QColor m_colorLine;

	bool m_bMeasureDistance;
	int m_nClicked;
	int m_point[2][2];

	QPoint m_textPos;
	QString m_str;
	bool m_bVertical;
	QColor m_titleColor;

	callback<void> DidEnter, DidLeave;
	callback<int> DidChangedHLine;
	callback<int> DidChangedVLine;
	callback<int> DidChangedRLine;
	callback2<int, int> DidClickedMouse;
	callback<void> DidDoubleClickedMouse;
	callback<QPoint&> DidMovedMouse;
};


#endif // QIMAGEVIEW_H
