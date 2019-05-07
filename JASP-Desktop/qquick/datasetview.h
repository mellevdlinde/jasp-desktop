#ifndef DATASETVIEW_H
#define DATASETVIEW_H

#include <QObject>
#include <QQuickItem>
#include <QAbstractTableModel>
#include <vector>
#include <stack>
#include <QSGFlatColorMaterial>

#include <map>
#include <QFontMetricsF>
#include <QtQml>
#include "utilities/qutils.h"

//#define DATASETVIEW_DEBUG_VIEWPORT
//#define DATASETVIEW_DEBUG_CREATION

struct ItemContextualized
{
	ItemContextualized(QQmlContext * context = nullptr, QQuickItem * item = nullptr) : item(item), context(context) {}

	QQuickItem * item;
	QQmlContext * context;
};



class DataSetView : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY( QAbstractTableModel * model READ model					WRITE setModel					NOTIFY modelChanged )
	Q_PROPERTY( float itemHorizontalPadding READ itemHorizontalPadding	WRITE setItemHorizontalPadding	NOTIFY itemHorizontalPaddingChanged )
	Q_PROPERTY( float itemVerticalPadding	READ itemVerticalPadding	WRITE setItemVerticalPadding	NOTIFY itemVerticalPaddingChanged )

	Q_PROPERTY( float viewportX				READ viewportX				WRITE setViewportX				NOTIFY viewportXChanged )
	Q_PROPERTY( float viewportY				READ viewportY				WRITE setViewportY				NOTIFY viewportYChanged )
	Q_PROPERTY( float viewportW				READ viewportW				WRITE setViewportW				NOTIFY viewportWChanged )
	Q_PROPERTY( float viewportH				READ viewportH				WRITE setViewportH				NOTIFY viewportHChanged )

	Q_PROPERTY( QQmlComponent * itemDelegate			READ itemDelegate			WRITE setItemDelegate			NOTIFY itemDelegateChanged )
	Q_PROPERTY( QQmlComponent * rowNumberDelegate		READ rowNumberDelegate		WRITE setRowNumberDelegate		NOTIFY rowNumberDelegateChanged )
	Q_PROPERTY( QQmlComponent * columnHeaderDelegate	READ columnHeaderDelegate	WRITE setColumnHeaderDelegate	NOTIFY columnHeaderDelegateChanged )

	Q_PROPERTY( QQuickItem * leftTopCornerItem			READ leftTopCornerItem		WRITE setLeftTopCornerItem		NOTIFY leftTopCornerItemChanged )
	Q_PROPERTY( QQuickItem * extraColumnItem			READ extraColumnItem		WRITE setExtraColumnItem		NOTIFY extraColumnItemChanged )

	Q_PROPERTY( QFont font	MEMBER _font NOTIFY fontChanged)

	Q_PROPERTY( float headerHeight		READ headerHeight							NOTIFY headerHeightChanged )
	Q_PROPERTY( float rowNumberWidth	READ rowNumberWidth							NOTIFY rowNumberWidthChanged )


public:
	DataSetView(QQuickItem *parent = nullptr);

	QAbstractTableModel * model() { return _model; }
	void setModel(QAbstractTableModel * model);

	float itemHorizontalPadding()	{ return _itemHorizontalPadding;}
	float itemVerticalPadding()		{ return _itemVerticalPadding;}

	float viewportX()				{ return _viewportX; }
	float viewportY()				{ return _viewportY; }
	float viewportW()				{ return _viewportW; }
	float viewportH()				{ return _viewportH; }

	QQmlComponent * itemDelegate()			{ return _itemDelegate; }
	QQmlComponent * rowNumberDelegate()		{ return _rowNumberDelegate; }
	QQmlComponent * columnHeaderDelegate()	{ return _columnHeaderDelegate; }

	QQuickItem * leftTopCornerItem()	{ return _leftTopItem; }
	QQuickItem * extraColumnItem()		{ return _extraColumnItem; }

	GENERIC_SET_FUNCTION(ViewportX, _viewportX, viewportXChanged, float)
	GENERIC_SET_FUNCTION(ViewportY, _viewportY, viewportYChanged, float)
	GENERIC_SET_FUNCTION(ViewportW, _viewportW, viewportWChanged, float)
	GENERIC_SET_FUNCTION(ViewportH, _viewportH, viewportHChanged, float)

	void setItemHorizontalPadding(float newHorizontalPadding)	{ if(newHorizontalPadding != _itemHorizontalPadding)	{ _itemHorizontalPadding = newHorizontalPadding;	emit itemHorizontalPaddingChanged();	update(); }}
	void setItemVerticalPadding(float newVerticalPadding)		{ if(newVerticalPadding != _itemVerticalPadding)		{ _itemVerticalPadding = newVerticalPadding;		emit itemVerticalPaddingChanged();		update(); }}

	void setRowNumberDelegate(QQmlComponent * newDelegate);
	void setColumnHeaderDelegate(QQmlComponent * newDelegate);
	void setItemDelegate(QQmlComponent * newDelegate);

	void setLeftTopCornerItem(QQuickItem * newItem);
	void setExtraColumnItem(QQuickItem * newItem);

	float headerHeight()		{ return _dataRowsMaxHeight; }
	float rowNumberWidth()		{ return _rowNumberMaxWidth; }

	GENERIC_SET_FUNCTION(HeaderHeight,		_dataRowsMaxHeight, headerHeightChanged,		float)
	GENERIC_SET_FUNCTION(RowNumberWidth,	_rowNumberMaxWidth, rowNumberWidthChanged,		float)

signals:
	void modelChanged();
	void fontPixelSizeChanged();
	void itemHorizontalPaddingChanged();
	void itemVerticalPaddingChanged();

	void viewportXChanged();
	void viewportYChanged();
	void viewportWChanged();
	void viewportHChanged();

	void rowNumberDelegateChanged();
	void columnHeaderDelegateChanged();
	void itemDelegateChanged();
	void leftTopCornerItemChanged();
	void extraColumnItemChanged();

	void itemSizeChanged();

	void fontChanged();

	void headerHeightChanged();
	void rowNumberWidthChanged();

public slots:
	void aContentSizeChanged() { _recalculateCellSizes = true; }
	void viewportChanged();
	void myParentChanged(QQuickItem *);

	void reloadTextItems();
	void reloadRowNumbers();
	void reloadColumnHeaders();

	void calculateCellSizes();

	void modelDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)	{ calculateCellSizes(); }
	void modelHeaderDataChanged(Qt::Orientation, int, int)									{ calculateCellSizes(); }
	void modelAboutToBeReset()																{ _storedLineFlags.clear(); _storedDisplayText.clear(); }
	void modelWasReset()																	{ setRolenames(); calculateCellSizes(); }

protected:
	void setRolenames();
	void determineCurrentViewPortIndices();
	void storeOutOfViewItems();
	void buildNewLinesAndCreateNewItems();

	QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
	float extraColumnWidth() { return _extraColumnItem == nullptr ? 0 : _extraColumnItem->width(); }

	QQuickItem * createTextItem(int row, int col);
	void storeTextItem(int row, int col, bool cleanUp = true);

	QQuickItem * createRowNumber(int row);
	void storeRowNumber(int row);

	QQuickItem * createColumnHeader(int col);
	void storeColumnHeader(int col);

	QQuickItem *	createleftTopCorner();
	void			updateExtraColumnItem();

	QQmlContext * setStyleDataItem(			QQmlContext * previousContext, bool active, size_t col, size_t row);
	QQmlContext * setStyleDataRowNumber(	QQmlContext * previousContext, QString text, int row);
	QQmlContext * setStyleDataColumnHeader(	QQmlContext * previousContext, QString text, int column, bool isComputed, bool isInvalidated, bool isFiltered,  QString computedError);

	void addLine(float x0, float y0, float x1, float y1);


protected:
	QAbstractTableModel *								_model = nullptr;

	std::vector<QSizeF>									_cellSizes; //[col]
	std::vector<float>									_colXPositions; //[col][row]
	std::vector<float>									_dataColsMaxWidth;
	std::stack<ItemContextualized*>						_textItemStorage;
	std::stack<ItemContextualized*>						_rowNumberStorage;
	std::map<int, ItemContextualized *>					_rowNumberItems;
	std::stack<ItemContextualized*>						_columnHeaderStorage;
	std::map<int, ItemContextualized *>					_columnHeaderItems;
	std::map<int, std::map<int, ItemContextualized *>>	_cellTextItems;			//[col][row]
	std::vector<float>									_lines;
	QQuickItem											*_leftTopItem = nullptr,
														*_extraColumnItem = nullptr;

	bool	_recalculateCellSizes	= false,
			_ignoreViewpoint		= true;

	float	_dataRowsMaxHeight,
			_itemHorizontalPadding	= 8,
			_itemVerticalPadding	= 8,
			_dataWidth				= -1;


	QQmlComponent	* _itemDelegate				= nullptr;
	QQmlComponent	* _rowNumberDelegate		= nullptr;
	QQmlComponent	* _columnHeaderDelegate		= nullptr;
	QQmlComponent	* _leftTopCornerDelegate	= nullptr;
	QQmlComponent	* _styleDataCreator			= nullptr;

	QSGFlatColorMaterial material;

	QFont _font;

	float _viewportX=0, _viewportY=0, _viewportW=1, _viewportH=1, _viewportReasonableMaximumW = 5000, _viewportReasonableMaximumH = 3000;
	int _previousViewportColMin = -1,
		_previousViewportColMax = -1,
		_previousViewportRowMin = -1,
		_previousViewportRowMax = -1,
		_viewportMargin			= 3,
		_currentViewportColMin	= -1,
		_currentViewportColMax	= -1,
		_currentViewportRowMin	= -1,
		_currentViewportRowMax	= -1;

	QFontMetricsF _metricsFont;

	std::map<std::string, int> _roleNameToRole;

	float	_rowNumberMaxWidth	= 0;
	bool	_linesWasChanged	= false;
	size_t	_linesActualSize	= 0;

	std::map<size_t, std::map<size_t, unsigned char>>	_storedLineFlags;
	std::map<size_t, std::map<size_t, QString>>			_storedDisplayText;
};



#endif // DATASETVIEW_H
