#include "traffxml/traff_source.hpp"

#include "traffxml/traff_model_xml.hpp"
#include "traffxml/traff_storage.hpp"

#include "geometry/rect2d.hpp"

#include "platform/platform.hpp"

#include <functional>
#include <thread>
#include <vector>

namespace traffxml {
TraffSource::TraffSource(TraffSourceManager & manager)
  : m_manager(manager)
{}

void TraffSource::SubscribeOrChangeSubscription(std::set<MwmSet::MwmId> const & mwms)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!IsSubscribed())
    Subscribe(mwms);
  else
    ChangeSubscription(mwms);
}

std::string TraffSource::GetMwmFilters(std::set<MwmSet::MwmId> const & mwms)
{
  std::vector<m2::RectD> rects;
  for (auto mwmId : mwms)
    rects.push_back(mwmId.GetInfo()->m_bordersRect);
  return traffxml::FiltersToXml(rects);
}

void MockTraffSource::Create(TraffSourceManager & manager)
{
  std::unique_ptr<MockTraffSource> source = std::unique_ptr<MockTraffSource>(new MockTraffSource(manager));
  manager.RegisterSource(std::move(source));
}

MockTraffSource::MockTraffSource(TraffSourceManager & manager)
  : TraffSource(manager)
{}

void MockTraffSource::Subscribe(std::set<MwmSet::MwmId> const & mwms)
{
  std::string filterList = GetMwmFilters(mwms);
  LOG(LINFO, ("Would subscribe to:\n", filterList));
  m_subscriptionId = "placeholder_subscription_id";
  m_nextRequestTime = std::chrono::steady_clock::now(); // would be in the future if we got a feed here
}

void MockTraffSource::ChangeSubscription(std::set<MwmSet::MwmId> const & mwms)
{
  if (!IsSubscribed())
    return;
  std::string filterList = GetMwmFilters(mwms);
  LOG(LINFO, ("Would change subscription", m_subscriptionId, "to:\n", filterList));
  m_nextRequestTime = std::chrono::steady_clock::now(); // would be in the future if we got a feed here
}

void MockTraffSource::Unsubscribe()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!IsSubscribed())
    return;
  LOG(LINFO, ("Would unsubscribe from", m_subscriptionId));
  m_subscriptionId.clear();
}

bool MockTraffSource::IsPollNeeded()
{
  return m_nextRequestTime.load() <= std::chrono::steady_clock::now();
}

void MockTraffSource::Poll()
{
  //std::string fileName("test_data/traff/PL-A18-Krzyzowa-Lipiany.xml");
  std::string fileName("test_data/traff/PL-A18-Krzyzowa-Lipiany-bidir.xml");
  //std::string fileName("test_data/traff/LT-A1-Vezaiciai-Endriejavas.xml");
  traffxml::LocalStorage storage(fileName);
  pugi::xml_document document;
  auto const load_result = storage.Load(document);
  if (!load_result)
    return;

  m_lastRequestTime = std::chrono::steady_clock::now();
  std::setlocale(LC_ALL, "en_US.UTF-8");
  traffxml::TraffFeed feed;
  if (traffxml::ParseTraff(document, std::nullopt /* dataSource */, feed))
  {
    m_lastResponseTime = std::chrono::steady_clock::now();
    m_nextRequestTime = std::chrono::steady_clock::now() + m_updateInterval;
    m_lastAvailability = Availability::IsAvailable;
    m_manager.ReceiveFeed(feed);
  }
  else
  {
    LOG(LWARNING, ("An error occurred parsing the TraFF feed"));
    m_lastAvailability = Availability::Error;
    /*
     * TODO how should we deal with future requests?
     * Static files usually don’t change.
     */
    m_nextRequestTime = std::chrono::steady_clock::now() + m_updateInterval;
  }
}

TraffResponse HttpPost(std::string const & url, std::string data)
{
  platform::HttpClient request(url);
  request.SetBodyData(data, "application/xml");

  if (!request.RunHttpRequest() || request.ErrorCode() != 200)
  {
    TraffResponse result;
    result.m_status = ResponseStatus::InternalError;
    return result;
  }

  LOG(LDEBUG, ("Got response, status", request.ErrorCode()));

  TraffResponse result = ParseResponse(request.ServerResponse());
  return result;
}

void HttpTraffSource::Create(TraffSourceManager & manager, std::string const & url)
{
  std::unique_ptr<HttpTraffSource> source = std::unique_ptr<HttpTraffSource>(new HttpTraffSource(manager, url));
  manager.RegisterSource(std::move(source));
}

HttpTraffSource::HttpTraffSource(TraffSourceManager & manager, std::string const & url)
  : TraffSource(manager)
  , m_url(url)
{}

void HttpTraffSource::Close()
{
  std::string data;
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_subscriptionId.empty())
      return;
    data = "<request operation=\"UNSUBSCRIBE\" subscription_id=\"" + m_subscriptionId + "\"/>";
    m_subscriptionId.clear();
  }

  LOG(LDEBUG, ("Sending request:\n", data));

  threads::SimpleThread thread([this, data]() {
    TraffResponse response = HttpPost(m_url, data);
    return;
  });
  thread.detach();
}

void HttpTraffSource::Subscribe(std::set<MwmSet::MwmId> const & mwms)
{
  std::string data = "<request operation=\"SUBSCRIBE\">\n<filter_list>\n"
      + GetMwmFilters(mwms)
      + "</filter_list>\n"
      + "</request>";
  LOG(LDEBUG, ("Sending request:\n", data));

  threads::SimpleThread thread([this, data]() {
    // TODO sometimes the request gets sent (and processed) twice
    TraffResponse response = HttpPost(m_url, data);
    OnSubscribeResponse(response);
    return;
  });
  thread.detach();
}

void HttpTraffSource::OnFeedReceived(TraffFeed const & feed)
{
  m_lastResponseTime = std::chrono::steady_clock::now();
  m_nextRequestTime = std::chrono::steady_clock::now() + m_updateInterval;
  m_lastAvailability = Availability::IsAvailable;
  m_manager.ReceiveFeed(feed);
}

void HttpTraffSource::OnSubscribeResponse(TraffResponse const & response)
{
  if (response.m_status == ResponseStatus::Ok
      || response.m_status == ResponseStatus::PartiallyCovered)
  {
    if (response.m_subscriptionId.empty())
      LOG(LWARNING, ("Server replied with", response.m_status, "but subscription ID is empty; ignoring"));
    else
    {
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscriptionId = response.m_subscriptionId;
        // TODO timeout
      }
      if (response.m_feed && !response.m_feed.value().empty())
        OnFeedReceived(response.m_feed.value());
      else
        Poll();
    }
  }
  else
    LOG(LWARNING, ("Subscribe request failed:", response.m_status));
}

void HttpTraffSource::ChangeSubscription(std::set<MwmSet::MwmId> const & mwms)
{
  std::string data = "<request operation=\"SUBSCRIPTION_CHANGE\" subscription_id=\"" + m_subscriptionId + "\">\n"
      + "<filter_list>\n"
      + GetMwmFilters(mwms)
      + "</filter_list>\n"
      + "</request>";
  LOG(LDEBUG, ("Sending request:\n", data));

  threads::SimpleThread thread([this, data]() {
    TraffResponse response = HttpPost(m_url, data);
    OnChangeSubscriptionResponse(response);
    return;
  });
  thread.detach();
}

void HttpTraffSource::OnChangeSubscriptionResponse(TraffResponse const & response)
{
  if (response.m_status == ResponseStatus::Ok
      || response.m_status == ResponseStatus::PartiallyCovered)
  {
    if (response.m_feed && !response.m_feed.value().empty())
      OnFeedReceived(response.m_feed.value());
    else
      Poll();
  }
  else if (response.m_status == ResponseStatus::SubscriptionUnknown)
  {
    LOG(LWARNING, ("Change Subscription returned", response.m_status, " – removing subscription", m_subscriptionId));
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_subscriptionId.clear();
    }
  }
  else
    LOG(LWARNING, ("Change Subscription request failed:", response.m_status));
}

void HttpTraffSource::Unsubscribe()
{
  std::string data;
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_subscriptionId.empty())
      return;
    data = "<request operation=\"UNSUBSCRIBE\" subscription_id=\"" + m_subscriptionId + "\"/>";
  }

  LOG(LDEBUG, ("Sending request:\n", data));

  threads::SimpleThread thread([this, data]() {
    TraffResponse response = HttpPost(m_url, data);
    OnUnsubscribeResponse(response);
    return;
  });
  thread.detach();
}

void HttpTraffSource::OnUnsubscribeResponse(TraffResponse const & response)
{
  if (response.m_status != ResponseStatus::Ok
      && response.m_status != ResponseStatus::SubscriptionUnknown)
  {
    LOG(LWARNING, ("Unsubscribe returned", response.m_status, " – removing subscription"));
  }
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscriptionId.clear();
  }
}

bool HttpTraffSource::IsPollNeeded()
{
  // TODO revisit logic
  return m_nextRequestTime.load() <= std::chrono::steady_clock::now();
}

void HttpTraffSource::Poll()
{
  std::string data;
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_subscriptionId.empty())
      return;
    data = "<request operation=\"POLL\" subscription_id=\"" + m_subscriptionId + "\"/>";
  }

  LOG(LDEBUG, ("Sending request:\n", data));

  threads::SimpleThread thread([this, data]() {
    // TODO sometimes the request gets sent (and processed) twice
    TraffResponse response = HttpPost(m_url, data);
    OnPollResponse(response);
    return;
  });
  thread.detach();
}

void HttpTraffSource::OnPollResponse(TraffResponse const & response)
{
  if (response.m_status == ResponseStatus::Ok)
  {
    if (response.m_feed && !response.m_feed.value().empty())
      OnFeedReceived(response.m_feed.value());
  }
  else if (response.m_status == ResponseStatus::SubscriptionUnknown)
  {
    LOG(LWARNING, ("Poll returned", response.m_status, " – removing subscription", m_subscriptionId));
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_subscriptionId.clear();
    }
  }
  else
    LOG(LWARNING, ("Poll returned", response.m_status));
}
}  // namespace traffxml
