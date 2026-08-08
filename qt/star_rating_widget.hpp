#pragma once

#include "indexer/reviews_display.hpp"

#include <QtWidgets/QWidget>

namespace qt
{
class StarRatingWidget : public QWidget
{
  float m_rating;

  static QPixmap const & fullStar()
  {
    static QPixmap const pixmap(":/reviews/star_full.png");
    return pixmap;
  }
  static QPixmap const & emptyStar()
  {
    static QPixmap const pixmap(":/reviews/star_empty.png");
    return pixmap;
  }

public:
  explicit StarRatingWidget(float const rating, QWidget * parent = nullptr)
    : QWidget(parent)
    , m_rating(std::clamp(rating, reviews::MIN_STAR_RATING, reviews::MAX_STAR_RATING))
  {
    setFixedSize(fullStar().width() * static_cast<int>(reviews::MAX_STAR_RATING), fullStar().height());
  }

protected:
  void paintEvent(QPaintEvent * event) override;
};
}  // namespace qt
