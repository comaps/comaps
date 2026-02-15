#include "qt/preferences_dialog.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "indexer/map_style.hpp"
#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include <QLocale>
#include <QtGui/QIcon>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

using namespace measurement_utils;

#ifdef BUILD_DESIGNER
std::string const kEnabledAutoRegenGeomIndex = "EnabledAutoRegenGeomIndex";
#endif

namespace qt
{
PreferencesDialog::PreferencesDialog(QWidget * parent, Framework & framework)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  QIcon icon(":/ui/logo.png");
  setWindowIcon(icon);
  setWindowTitle(tr("Preferences"));

  QButtonGroup * unitsGroup = new QButtonGroup(this);
  QGroupBox * unitsRadioBox = new QGroupBox("System of measurement");
  {
    QHBoxLayout * layout = new QHBoxLayout();

    QRadioButton * radioButton = new QRadioButton("Metric");
    layout->addWidget(radioButton);
    unitsGroup->addButton(radioButton, static_cast<int>(Units::Metric));

    radioButton = new QRadioButton("Imperial (foot)");
    layout->addWidget(radioButton);
    unitsGroup->addButton(radioButton, static_cast<int>(Units::Imperial));

    unitsRadioBox->setLayout(layout);

    Units u;
    if (!settings::Get(settings::kMeasurementUnits, u))
    {
      // Set default measurement from system locale
      if (QLocale::system().measurementSystem() == QLocale::MetricSystem)
        u = Units::Metric;
      else
        u = Units::Imperial;
    }
    unitsGroup->button(static_cast<int>(u))->setChecked(true);

    // Temporary to pass the address of overloaded function.
    void (QButtonGroup::*buttonClicked)(int) = &QButtonGroup::idClicked;
    connect(unitsGroup, buttonClicked, [&framework](int i)
    {
      Units u = Units::Metric;
      switch (i)
      {
      case 0: u = Units::Metric; break;
      case 1: u = Units::Imperial; break;
      }

      settings::Set(settings::kMeasurementUnits, u);
      framework.SetupMeasurementSystem();
    });
  }

  QCheckBox * largeFontCheckBox = new QCheckBox("Use larger font on the map");
  {
    largeFontCheckBox->setChecked(framework.LoadLargeFontsSize());
    connect(largeFontCheckBox, &QCheckBox::stateChanged,
            [&framework](int i) { framework.SetLargeFontsSize(static_cast<bool>(i)); });
  }

  QCheckBox * transliterationCheckBox = new QCheckBox("Transliterate to Latin");
  {
    transliterationCheckBox->setChecked(framework.LoadTransliteration());
    connect(transliterationCheckBox, &QCheckBox::stateChanged, [&framework](int i)
    {
      bool const enable = i > 0;
      framework.SaveTransliteration(enable);
      framework.AllowTransliteration(enable);
    });
  }

  QCheckBox * developerModeCheckBox = new QCheckBox("Developer Mode");
  {
    bool developerMode;
    if (settings::Get(settings::kDeveloperMode, developerMode) && developerMode)
      developerModeCheckBox->setChecked(developerMode);
    connect(developerModeCheckBox, &QCheckBox::stateChanged,
            [](int i) { settings::Set(settings::kDeveloperMode, static_cast<bool>(i)); });
  }

  QLabel * mapLanguageLabel = new QLabel("Map Language");
  QComboBox * mapLanguageComboBox = new QComboBox();
  {
    // The property maxVisibleItems is ignored for non-editable comboboxes in styles that
    // return true for `QStyle::SH_ComboBox_Popup such as the Mac style or the Gtk+ Style.
    // So we ensure that it returns false here.
    mapLanguageComboBox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    mapLanguageComboBox->setMaxVisibleItems(10);
    StringUtf8Multilang::Languages const & supportedLanguages =
        StringUtf8Multilang::GetSupportedLanguages(/* includeServiceLangs */ false);

    // Create a vector of pairs (name, index) and sort by name
    std::vector<std::pair<std::string, size_t>> languageNameIndexPairs;
    for (size_t i = 0; i < supportedLanguages.size(); ++i)
    {
      languageNameIndexPairs.emplace_back(std::string(supportedLanguages[i].m_name), i);
    }
    std::sort(languageNameIndexPairs.begin(), languageNameIndexPairs.end(),
              [](auto const & a, auto const & b) { return a.first < b.first; });

    QStringList languagesList = QStringList();
    std::vector<size_t> sortedIndices;
    languagesList << QString::fromStdString("Local Language");
    sortedIndices.push_back(0);
    for (auto const & pair : languageNameIndexPairs)
    {
      languagesList << QString::fromStdString(pair.first);
      sortedIndices.push_back(pair.second);
    }

    mapLanguageComboBox->addItems(languagesList);
    std::string const & mapLanguageCode = framework.GetMapLanguageCode();
    int8_t languageIndex = StringUtf8Multilang::GetLangIndex(mapLanguageCode);
    if (languageIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      languageIndex = StringUtf8Multilang::kDefaultCode;

    mapLanguageComboBox->setCurrentText(
        QString::fromStdString(std::string(StringUtf8Multilang::GetLangNameByCode(languageIndex))));
    connect(mapLanguageComboBox, &QComboBox::activated, [&framework, &supportedLanguages, sortedIndices](int index)
    {
      if (index == 0)
      {
        framework.SetMapLanguageCode("default");
        return;
      }
      auto const & mapLanguageCode = std::string(supportedLanguages[sortedIndices[index]].m_code);
      framework.SetMapLanguageCode(mapLanguageCode);
    });
  }

  QButtonGroup * nightModeGroup = new QButtonGroup(this);
  QGroupBox * nightModeRadioBox = new QGroupBox("Night Mode");
  {
    using namespace style_utils;
    QHBoxLayout * layout = new QHBoxLayout();

    QRadioButton * radioButton = new QRadioButton("Off");
    layout->addWidget(radioButton);
    nightModeGroup->addButton(radioButton, static_cast<int>(NightMode::Off));

    radioButton = new QRadioButton("On");
    layout->addWidget(radioButton);
    nightModeGroup->addButton(radioButton, static_cast<int>(NightMode::On));

    nightModeRadioBox->setLayout(layout);

    int const btn = MapStyleIsDark(framework.GetMapStyle()) ? 1 : 0;
    nightModeGroup->button(btn)->setChecked(true);

    void (QButtonGroup::*buttonClicked)(int) = &QButtonGroup::idClicked;
    connect(nightModeGroup, buttonClicked, [&framework](int i)
    {
      auto const currStyle = framework.GetMapStyle();
      framework.SetMapStyle((i == 0) ? GetLightMapStyleVariant(currStyle) : GetDarkMapStyleVariant(currStyle));
    });
  }

#ifdef BUILD_DESIGNER
  QCheckBox * indexRegenCheckBox = new QCheckBox("Enable auto regeneration of geometry index");
  {
    bool enabled = false;
    if (!settings::Get(kEnabledAutoRegenGeomIndex, enabled))
      settings::Set(kEnabledAutoRegenGeomIndex, false);
    indexRegenCheckBox->setChecked(enabled);
    connect(indexRegenCheckBox, &QCheckBox::stateChanged,
            [](int i) { settings::Set(kEnabledAutoRegenGeomIndex, static_cast<bool>(i)); });
  }
#endif

  QHBoxLayout * bottomLayout = new QHBoxLayout();
  {
    QPushButton * closeButton = new QPushButton(tr("Close"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeButton->setDefault(true);
    connect(closeButton, &QAbstractButton::clicked, [this]() { done(0); });

    bottomLayout->addStretch(1);
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(closeButton);
  }

  QVBoxLayout * finalLayout = new QVBoxLayout();
  finalLayout->addWidget(unitsRadioBox);
  finalLayout->addWidget(largeFontCheckBox);
  finalLayout->addWidget(transliterationCheckBox);
  finalLayout->addWidget(developerModeCheckBox);
  finalLayout->addWidget(mapLanguageLabel);
  finalLayout->addWidget(mapLanguageComboBox);
  finalLayout->addWidget(nightModeRadioBox);
#ifdef BUILD_DESIGNER
  finalLayout->addWidget(indexRegenCheckBox);
#endif
  finalLayout->addLayout(bottomLayout);
  setLayout(finalLayout);
}
}  // namespace qt
