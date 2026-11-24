#include "testing/testing.hpp"

#include "editor/osm_auth.hpp"
#include "platform/platform.hpp"

#include <string>

namespace osm_auth
{
using osm::OsmOAuth;

char const * kValidOsmUser = "CoMapsTestUser";
char const * kValidOsmPassword = "12345678";
static constexpr char const * kInvalidOsmPassword = "123";
static constexpr char const * kForgotPasswordEmail = "osmtest1@comaps.app";

UNIT_TEST(OSM_Auth_InvalidLogin)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kValidOsmUser, kInvalidOsmPassword), ());
  TEST_EQUAL(result, false, ("invalid password"));
  TEST(!auth.IsAuthorized(), ("Should not be authorized."));
}

UNIT_TEST(OSM_Auth_Login)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.AuthorizePassword(kValidOsmUser, kValidOsmPassword), ());
  TEST_EQUAL(result, true, ("login to test server"));
  TEST(auth.IsAuthorized(), ("Should be authorized."));
  OsmOAuth::Response const perm = auth.Request("/permissions");
  TEST_EQUAL(perm.first, OsmOAuth::HTTP::OK, ("permission request ok"));
  TEST_NOT_EQUAL(perm.second.find("write_api"), std::string::npos, ("can write to api"));
}

/*
UNIT_TEST(OSM_Auth_ForgotPassword)
{
  OsmOAuth auth = OsmOAuth::DevServerAuth();
  bool result;
  TEST_NO_THROW(result = auth.ResetPassword(kForgotPasswordEmail), ());
  TEST_EQUAL(result, true, ("Correct email"));
  TEST_NO_THROW(result = auth.ResetPassword("not@registered.email"), ());
  TEST_EQUAL(result, false, ("Incorrect email"));
}
*/


UNIT_TEST(OAuth_Token_parse)
{
  auto token = osm::FindAuthenticityToken("/oauth2/authorize",
  R"(<div class="col-auto mx-1"><form action="/oauth2/authorize" accept-charset="UTF-8" method="post">
    <input type="hidden" name="authenticity_token" value="J5senhh" />
    <input value="cm://oauth2/osm/callback" type="hidden" name="redirect_uri" />
    <input type="submit" name="commit" value="Authorize" class="btn btn-primary" />
  </form></div>)");

  TEST_EQUAL(token, "J5senhh", ("Invalid token"));

  // Two tokens. One for "Allow" other for "Deny". Always pick the first one.
  token = osm::FindAuthenticityToken("/oauth2/authorize",
  R"(<div class="col-auto mx-1"><form action="/oauth2/authorize" accept-charset="UTF-8" method="post">
    <input type="hidden" name="authenticity_token" value="lRFbEQN0d2r7XeFq" />
    <input value="cm://oauth2/osm/callback" type="hidden" name="redirect_uri" />
    <input type="submit" name="commit" value="Authorize" class="btn btn-primary" />
  </form></div>
  <div class="col-auto mx-1"><form action="/oauth2/authorize" accept-charset="UTF-8" method="post">
    <input type="hidden" name="_method" value="delete" />
    <input type="hidden" name="authenticity_token" value="J5senhh" />
    <input value="cm://oauth2/osm/callback" type="hidden" name="redirect_uri" />
    <input type="submit" name="commit" value="Deny" class="btn btn-secondary" />
  </form></div>)");

  TEST_EQUAL(token, "lRFbEQN0d2r7XeFq", ("Invalid token"));

  // Three forms: one for language, two for OAuth
  token = osm::FindAuthenticityToken("/oauth2/authorize",
  R"(<div class="modal-body px-1"><form action="/preferences/basic" accept-charset="UTF-8" method="post">
    <input type="hidden" name="_method" value="put" />
    <input type="hidden" name="authenticity_token" value="8h-snCINM3O" />
  </form></div>\n"   "<div class="col-auto mx-1"><form action="/oauth2/authorize" accept-charset="UTF-8" method="post">
    <input type="hidden" name="authenticity_token" value="8DDeqYcFHUIjw" />
    <input value="cm://oauth2/osm/callback" type="hidden" name="redirect_uri" />
    <input type="submit" name="commit" value="Authorize" class="btn btn-primary" />
  </form></div>
  <div class="col-auto mx-1"><form action="/oauth2/authorize" accept-charset="UTF-8" method="post">
    <input type="hidden" name="_method" value="delete" />
    <input type="hidden" name="authenticity_token" value="4RbveLjuvOXlok9Q" />
    <input value="cm://oauth2/osm/callback" type="hidden" name="redirect_uri" />
    <input type="submit" name="commit" value="Deny" class="btn btn-secondary" />
  </form></div>)");

  TEST_EQUAL(token, "8DDeqYcFHUIjw", ("Invalid token"));
}

UNIT_TEST(OAuth_FindOauthCode) {
  auto code = osm::FindOauthCode("cm://oauth2/osm/callback?redirect=true&code=befd_095315f197f4");
  TEST_EQUAL(code, "befd_095315f197f4", ("Invalid code"));

  code = osm::FindOauthCode("cm://oauth2/osm/callback?code=45c023d6");
  TEST_EQUAL(code, "45c023d6", ("Invalid code"));

  code = osm::FindOauthCode("cm://oauth2/osm/callback?the_code=cant_find");
  TEST_EQUAL(code, std::string{}, ("Code should not be found"));
}

UNIT_TEST(OAuth_FindAccessToken) {
  auto token = osm::FindAccessToken(R"({"access_token":"vAK5XMBJeUDsF3xxFHt0", "token_type":"Bearer", "scope":"read_prefs"})");
  TEST_EQUAL(token, "vAK5XMBJeUDsF3xxFHt0", ("Invalid access_token"));

  token = osm::FindAccessToken(R"({"token_type":"Bearer", "scope":"read_prefs", "access_token":"W1xgY0CtTDz0klQGa4Yp"})");
  TEST_EQUAL(token, "W1xgY0CtTDz0klQGa4Yp", ("Invalid access_token"));

  token = osm::FindAccessToken(R"({"token_type":"Bearer", "scope":"read_prefs", "some_other_token":"9cMyxvsOrM"})");
  TEST_EQUAL(token, std::string{}, ("access_token should not be found"));
}

UNIT_TEST(OAuth_PkceChallengeS256_RfcVector)
{
  // RFC 7636 Appendix B.
  std::string const verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
  TEST_EQUAL(osm::PkceChallengeS256(verifier), "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM", ());
}

UNIT_TEST(OAuth_ConsentPost_CarriesChallengeAndState)
{
  // The consent POST must bind the issued code to this flow or the later token exchange fails
  std::string const verifier = osm::GeneratePkceCodeVerifier();
  std::string const body =
      osm::BuildConsentPostRequest("tok", "cid", "uri", "scope", verifier, "flowstate123");
  TEST(body.find("code_challenge=" + osm::PkceChallengeS256(verifier)) != std::string::npos, (body));
  TEST(body.find("code_challenge_method=S256") != std::string::npos, (body));
  TEST(body.find("state=flowstate123") != std::string::npos, (body));

  // A plain grant with no in-flight flow carries neither the challenge nor state
  std::string const plain = osm::BuildConsentPostRequest("tok", "cid", "uri", "scope", "", "");
  TEST(plain.find("code_challenge") == std::string::npos, (plain));
  TEST(plain.find("state=") == std::string::npos, (plain));
}

UNIT_TEST(OAuth_PkceCodeVerifier_Format)
{
  // RFC 7636 section 4.1 wants 43 to 128 chars from the unreserved set [A-Za-z0-9-._~]
  std::string const v = osm::GeneratePkceCodeVerifier();
  TEST_GREATER_OR_EQUAL(v.size(), 43, ());
  TEST_LESS_OR_EQUAL(v.size(), 128, ());
  for (char c : v)
  {
    bool const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
                    c == '.' || c == '_' || c == '~';
    TEST(ok, ("non-unreserved char in verifier", c));
  }
  // Two successive verifiers must differ (decent randomness, not a constant).
  TEST_NOT_EQUAL(v, osm::GeneratePkceCodeVerifier(), ());
}

UNIT_TEST(OAuth_BuildUrl_CarriesPkceChallenge)
{
  OsmOAuth auth("client", "read_prefs write_api", "cm://oauth2/osm/callback", "https://example.invalid", "https://example.invalid");
  std::string const url = auth.BuildOAuth2Url();
  TEST_NOT_EQUAL(url.find("code_challenge_method=S256"), std::string::npos, ("auth url must request S256 PKCE", url));
  TEST_NOT_EQUAL(url.find("code_challenge="), std::string::npos, ("auth url must have a code_challenge", url));
}

UNIT_TEST(OAuth_State_RejectsUnsolicited)
{
  OsmOAuth auth("client", "read_prefs write_api", "cm://oauth2/osm/callback", "https://example.invalid", "https://example.invalid");
  std::string const url = auth.BuildOAuth2Url();

  // The authorize URL must have a CSRF state.
  auto const statePos = url.find("state=");
  TEST_NOT_EQUAL(statePos, std::string::npos, ("auth url must have a state", url));
  std::string state = url.substr(statePos + 6);
  auto const amp = state.find('&');
  if (amp != std::string::npos)
    state.erase(amp);
  TEST(!state.empty(), ("state must be non-empty", url));

  // Unsolicited or injected callbacks are rejected
  TEST(osm::IsPendingOAuth2State(state), ("the started flow's own state must validate"));
  TEST(!osm::IsPendingOAuth2State(state + "x"), ("a mismatching state must be rejected"));
  TEST(!osm::IsPendingOAuth2State(""), ("an empty state must be rejected"));
}
}  // namespace osm_auth
