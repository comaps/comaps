#pragma once

#include "map/place_page_info.hpp"

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QProgressBar>

#include <functional>

namespace place_page_dialog
{
enum PressedButton : int
{
  Close = QDialog::Rejected,
  RouteFrom,
  AddStop,
  RouteTo,
  EditPlace
};

void addCommonButtons(QDialog * this_, QDialogButtonBox * dbb, bool shouldShowEditPlace);

void resolveReviewEditorUrl(QDialog * this_, place_page::Info const & info, QProgressBar * spinner,
                            std::function<void(std::string const &)> const & onResolved,
                            std::function<void()> const & onEmpty);
}  // namespace place_page_dialog
