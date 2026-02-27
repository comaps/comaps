#include "indexer/ftypes_subtypes.hpp"

#include "base/assert.hpp"
#include "coding/csv_reader.hpp"
#include "coding/reader_streambuf.hpp"
#include "platform/platform.hpp"

namespace ftypes
{
/// Constructor
Subtypes::Subtypes()
{
  auto const & classificator = classif();

  // Get the stream to the CSV file.
  Platform & platform = GetPlatform();
  unique_ptr<ModelReader> reader = platform.GetReader(SUBTYPES_FILE);
  ReaderStreamBuf buffer(std::move(reader));
  istream stream(&buffer);

  // Load the CSV file and go through the lines of it one by one.
  for (auto const & columns : coding::CSVRunner(coding::CSVReader(stream, true, ';')))
  {
    // Skip empty lines.
    if (columns.empty())
      continue;

    // There only should be two columns.
    if (columns.size() != 2)
    {
      ASSERT(false, ("Parsing of subtypes file: Invalid columns \"", columns, "\""));
      break;
    }

    // Parse the column. The first column has the type definitions(s) and the second one has the associated subtype
    // definitions(s).
    vector<uint32_t> types;
    vector<uint32_t> subtypes;
    for (int columnIndex = 0; columnIndex < 2; columnIndex++)
    {
      string_view const column = columns[columnIndex];

      // Separate the different type definitions by the `,`. There needs to be at least one.
      vector<string_view> const typeDefinitions = strings::Tokenize(column, ",");
      if (typeDefinitions.size() < 1)
      {
        ASSERT(columnIndex != 0, ("Parsing of subtypes file: Invalid or missing types definition \"", column, "\""));
        ASSERT(columnIndex == 0, ("Parsing of subtypes file: Invalid or missing subtypes definition \"", column, "\""));
        break;
      }

      // Parse the type definitions and convert them to actual types. Invalid types are getting skipped.
      vector<uint32_t> typesInColumn;
      for (auto typeDefinition : typeDefinitions)
      {
        vector<string_view> const typePath = strings::Tokenize(typeDefinition, "|");
        uint32_t const type = classificator.GetTypeByPathSafe(typePath);
        if (type != IndexAndTypeMapping::INVALID_TYPE)
        {
          typesInColumn.push_back(type);

          vector<string> typesAndSubtypesPath(typePath.begin(), typePath.end());
          if (find(m_typesAndSubtypesPaths.begin(), m_typesAndSubtypesPaths.end(), typesAndSubtypesPath) ==
              m_typesAndSubtypesPaths.end())
            m_typesAndSubtypesPaths.push_back(typesAndSubtypesPath);
        }
        else
        {
          ASSERT(columnIndex != 0, ("Parsing of subtypes file: Invalid type \"", typeDefinition, "\""));
          ASSERT(columnIndex == 0, ("Parsing of subtypes file: Invalid subtype \"", typeDefinition, "\""));
        }
      }

      if (columnIndex == 0)
        types = typesInColumn;
      else
        subtypes = typesInColumn;
    }

    for (auto type : types)
    {
      m_types.insert(type);
      m_typesWithSubtypes[type] = subtypes;
    }

    for (auto subtype : subtypes)
      m_subtypes.insert(subtype);
  }
}

/// Static instance
Subtypes const & Subtypes::Instance()
{
  static Subtypes instance;
  return instance;
}
}  // namespace ftypes
