#include "qt/place_page_dialog_common.hpp"

#include "editor/review.hpp"

#include "map/place_page_info.hpp"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>

#include <optional>
#include <string>

namespace place_page_dialog
{
void addCommonButtons(QDialog * this_, QDialogButtonBox * dbb, bool shouldShowEditPlace)
{
  dbb->setCenterButtons(true);

  QPushButton * fromButton = new QPushButton("Route From");
  fromButton->setIcon(QIcon(":/navig64/point-start.png"));
  fromButton->setAutoDefault(false);
  this_->connect(fromButton, &QAbstractButton::clicked, this_, [this_] { this_->done(RouteFrom); });
  dbb->addButton(fromButton, QDialogButtonBox::ActionRole);

  QPushButton * addStopButton = new QPushButton("Add Stop");
  addStopButton->setIcon(QIcon(":/navig64/point-intermediate.png"));
  addStopButton->setAutoDefault(false);
  this_->connect(addStopButton, &QAbstractButton::clicked, this_, [this_] { this_->done(AddStop); });
  dbb->addButton(addStopButton, QDialogButtonBox::ActionRole);

  QPushButton * routeToButton = new QPushButton("Route To");
  routeToButton->setIcon(QIcon(":/navig64/point-finish.png"));
  routeToButton->setAutoDefault(false);
  this_->connect(routeToButton, &QAbstractButton::clicked, this_, [this_] { this_->done(RouteTo); });
  dbb->addButton(routeToButton, QDialogButtonBox::ActionRole);

  QPushButton * closeButton = new QPushButton("Close");
  closeButton->setDefault(true);
  this_->connect(closeButton, &QAbstractButton::clicked, this_, [this_] { this_->done(place_page_dialog::Close); });
  dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

  if (shouldShowEditPlace)
  {
    QPushButton * editButton = new QPushButton("Edit Place");
    this_->connect(editButton, &QAbstractButton::clicked, this_,
                   [this_] { this_->done(place_page_dialog::EditPlace); });
    dbb->addButton(editButton, QDialogButtonBox::AcceptRole);
  }
}

template <typename F1, typename F2>
void resolveReviewEditorUrl(QDialog * this_, place_page::Info const & info, QProgressBar * const spinner,
                            F1 && onResolved, F2 && onEmpty)
{
  auto * const watcher = new QFutureWatcher<std::optional<std::string>>(this_);
  this_->connect(watcher, &QFutureWatcher<std::optional<std::string>>::finished, this_, [=]()
  {
    spinner->hide();
    if (auto const & reviewUrl = watcher->result(); reviewUrl.has_value())
      onResolved(reviewUrl.value());
    else
      onEmpty();
  });
  QFuture<std::optional<std::string>> const reviewUrlFuture = QtConcurrent::run(reviews::GetReviewEditorUrl, info);
  watcher->setFuture(reviewUrlFuture);
}

}  // namespace place_page_dialog
