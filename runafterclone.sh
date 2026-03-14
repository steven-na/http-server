#!/usr/bin/env bash

echp "Checklist: "
cp ./Caddyfile.example ./Caddyfile
echo "- Update 'Caddyfile'"
cp ./add_post.sh.example ./add_post.sh
echo "- Update 'add_post.sh'"
cp ./compose.yaml.example ./compose.yaml
echo "- Update 'add_post.sh'"
cp ./pages/index.html.example ./pages/index.html
echo "- Update 'pages/index.html'"
cp ./pages/404.html.example ./pages/404.html
echo "- Update 'pages/404.html'"
cp ./templates/post.html.example ./templates/post.html
echo "- Update 'templates/post.html'"

cp ./posts/posts.txt.example ./posts/posts.txt

