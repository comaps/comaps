package app.organicmaps.sdk.util;

public final class Constants
{
  public static final int KB = 1024;
  public static final int MB = 1024 * 1024;
  public static final int GB = 1024 * 1024 * 1024;

  static final int READ_TIMEOUT_MS = 10000;

  public static class Url
  {
    public static final String SHORT_SHARE_PREFIX = "cm://";
    public static final String HTTP_SHARE_PREFIX = "https://comaps.at/";

    public static final String MAILTO_SCHEME = "mailto:";
    public static final String MAIL_SUBJECT = "?subject=";
    public static final String MAIL_BODY = "&body=";

    public static final String MATRIX = "https://matrix.to/#/%23comaps:matrix.org";
    public static final String MASTODON = "https://floss.social/@comaps";
    public static final String LEMMY = "https://sopuli.xyz/c/CoMaps";
    public static final String BLUESKY = "https://bsky.app/profile/comaps.app";
    public static final String PIXELFED = "https://pixelfed.social/comaps";
    public static final String FB_OM_COMMUNITY_HTTP = "https://www.facebook.com/profile.php?id=61575286559965";
    public static final String FB_OM_COMMUNITY_NATIVE = "fb://profile/61575286559965";

    public static final String CODE_REPO = "https://codeberg.org/comaps/comaps";

    public static final String COPYRIGHT = "file:///android_asset/copyright.html";
    public static final String FAQ = "file:///android_asset/faq.html";
    public static final String OPENING_HOURS_MANUAL = "file:///android_asset/opening_hours_how_to_edit.html";

    public static final String OSM_REGISTER = "https://www.openstreetmap.org/user/new";

    private Url() {}
  }

  public static class Package
  {
    public static final String FB_PACKAGE = "com.facebook.katana";

    private Package() {}
  }

  public static class Vendor
  {
    public static final String HUAWEI = "HUAWEI";
    public static final String XIAOMI = "XIAOMI";
  }

  private Constants() {}
}
