#pragma once

#include "traff_assessment_tool/resumable_timer.hpp"

#include <QLabel>
#include <QProgressBar>
#include <QtWidgets/QStyledItemDelegate>

class QAbstractItemModel;
class QComboBox;
class QTableView;
class QWidget;

namespace traffxml
{
class TrafficModel;

class ComboBoxDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  ComboBoxDelegate(QObject * parent = 0);

  QWidget * createEditor(QWidget * parent, QStyleOptionViewItem const & option,
                         QModelIndex const & index) const Q_DECL_OVERRIDE;

  void setEditorData(QWidget * editor, QModelIndex const & index) const Q_DECL_OVERRIDE;

  void setModelData(QWidget * editor, QAbstractItemModel * model,
                    QModelIndex const & index) const Q_DECL_OVERRIDE;

  void updateEditorGeometry(QWidget * editor, QStyleOptionViewItem const & option,
                            QModelIndex const & index) const Q_DECL_OVERRIDE;
};

class TrafficPanel : public QWidget
{
  Q_OBJECT

public:
  explicit TrafficPanel(TrafficModel * trafficModel, QWidget * parent);
  base::ResumableTimer & GetTimer() { return m_timer; }
  void SetStatus(bool inProgress, std::optional<size_t> messageCount = std::nullopt);

private:
  void CreateTable(TrafficModel * trafficModel);
  void FillTable();

signals:

public slots:
  // void OnCheckBoxClicked(int row, int state);

private:
  QTableView * m_table = Q_NULLPTR;
  QProgressBar * m_progressBar = Q_NULLPTR;
  base::ResumableTimer m_timer;
  QLabel * m_status = Q_NULLPTR;
};
}  // namespace traffxml
