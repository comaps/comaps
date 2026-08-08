#include "qt/star_rating_widget.hpp"

#include <QtGui/QPainter>

void qt::StarRatingWidget::paintEvent(QPaintEvent * event)
{
  Q_UNUSED(event);
  QPainter painter(this);

  int const starWidth = fullStar().width();
  int const starHeight = fullStar().height();
  constexpr int numStars = static_cast<int>(reviews::MAX_STAR_RATING);

  for (int i = 0; i < numStars; ++i) {
    int const xPos = i * starWidth;

    float fillLevel = m_rating - static_cast<float>(i);
    fillLevel = std::clamp(fillLevel, 0.0f, 1.0f);
    painter.drawPixmap(xPos, 0, emptyStar());
    if (fillLevel > 0) {
      int const fillWidth = static_cast<int>(static_cast<float>(starWidth) * fillLevel);
      painter.save();
      painter.setClipRect(xPos, 0, fillWidth, starHeight);
      painter.drawPixmap(xPos, 0, fullStar());
      painter.restore();
    }
  }
}
