#pragma once

#include "map/place_page_info.hpp"

#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QProgressBar>

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

template <typename F1, typename F2>
void resolveReviewEditorUrl(QDialog * this_, place_page::Info const & info, QProgressBar * spinner, F1 && onResolved,
                            F2 && onEmpty);
}  // namespace place_page_dialog
