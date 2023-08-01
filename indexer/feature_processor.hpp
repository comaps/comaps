#pragma once

#include "indexer/features_vector.hpp"

#include "coding/file_reader.hpp"
#include "coding/files_container.hpp"

#include <memory>
#include <string>

namespace feature
{
template <class ToDo>
void ForEachFeature(FilesContainerR const & cont, ToDo && toDo)
{
  FeaturesVectorTest features(cont);
  features.GetVector().ForEach(toDo);
}

template <class ToDo>
void ForEachFeature(ModelReaderPtr reader, ToDo && toDo)
{
  ForEachFeature(FilesContainerR(reader), toDo);
}

template <class ToDo>
void ForEachFeature(std::string const & fPath, ToDo && toDo)
{
  ForEachFeature(std::make_unique<FileReader>(fPath), toDo);
}
}  // namespace feature
