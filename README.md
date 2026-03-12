# C HTTP Server
## Overview
A simple, small C server that can be deployed with docker. It allows for quickly uploading blog posts, updating an RSS feed, and deployyment with docker, caddy, and cloudflare ddns.
The root url redirects to `/home/`, posts can be viewed at `/posts/{post_name}`, and the RSS feed can be viewed at `/rss.xml`

## Usage
- copy all of the `.example` files and remove `.example`. Input the correctt information for your site/cloudflare account.
- `docker compose up -d --build` will run the server once all of the information is correct.
- `./add_post {args}` will add posts to the site that can be viewed on `/posts/{post_name}`

## Planned features
- [ ] Email subscriptions
- [ ] Threaded http responses

## Attributions
- Initial versions based on [an article](https://bruinsslot.jp/post/simple-http-webserver-in-c/) by Jan Pieter Bruins Slot
