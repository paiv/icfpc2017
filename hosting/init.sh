#!/bin/bash
set -e

PUBLIC_URL="$1"

if [ -z "$PUBLIC_URL" ]; then
    echo 'usage: ./init.sh public_url'
    exit 1
fi


sed -e "s/TEMPLATE_HOST/$PUBLIC_URL/g" docker-compose.template.yml > docker-compose.yml


# pip install -r requirements.txt
# docker-compose run -d --build
