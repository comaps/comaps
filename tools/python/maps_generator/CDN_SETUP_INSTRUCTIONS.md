Edit the rclone conf secret for Codeberg Actions, to deliver maps to i.e. /var/www/html/maps/251231 via a limited user.

apt update
apt install nginx vim

### set hostname for ssh sanity (will show in console upon next bash launch):
vim /etc/hostname
hostname cdn-XX-1

### for SSL:
sudo snap install --classic certbot
sudo certbot --nginx

### remove IPs from logging on line ~36:
vim /etc/nginx/nginx.conf

```
        ##
        # Logging Settings
        ##
        log_format comaps '0.0.0.0 - - [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"';
        access_log /var/log/nginx/access.log comaps;
```

### set up monitoring:
apt install goaccess
edit /etc/goaccess/goaccess.conf and uncomment time-format %H:%M:%S, date-format %Y-%m-%d, log-format COMBINED
vim /etc/crontab

`*/5 *   * * *   root    /usr/bin/goaccess /var/log/nginx/access.log -o /var/www/html/monitor.html`

### set up basic http pages/responses:
cd /var/www/html/
mkdir maps
rm index.nginx-debian.html
wget https://www.comaps.app/favicon.ico
vim robots.txt

```
User-agent: *
Disallow: /
```

vim index.html

```
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">

<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <title>CoMaps CDN</title>
</head>

<body>
  <h1>This is a CDN for <a href="https://comaps.app">CoMaps</a></h1>

  <h2>Resources:</h2>
  <ol>
    <li>CoMaps <a href="https://cdn.comaps.app/subway/">subway validator</a></li>
    <li>CoMaps <a href="https://comaps.app/news/">News</a></li>
    <li><a href="https://comaps.app/donate/">Donate</a></li>
  </ol>
</body>
</html>
```
