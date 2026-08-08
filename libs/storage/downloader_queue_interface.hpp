#pragma once

#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

namespace storage
{
class QueueInterface
{
public:
  using ForEachCountryFunction = std::function<void(QueuedCountry const & country)>;
  using ForEachCountryMutable = std::function<void(QueuedCountry & country)>;

  virtual bool IsEmpty() const = 0;
  virtual size_t Count() const = 0;
  virtual bool Contains(CountryId const & country) const = 0;
  virtual void ForEachCountry(ForEachCountryFunction const & fn) const = 0;
  virtual void ForEachCountry(ForEachCountryMutable const & fn) = 0;

protected:
  virtual ~QueueInterface() = default;
};
}  // namespace storage
