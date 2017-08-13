
Hosting Punting Server
======================

### Requirements

- git
- docker
- docker-compose


### Install

Clone source code

```sh
git clone https://github.com/paiv/icfpc2017.git
```

Generate app declaration for your domain

```sh
cd icfpc2017/hosting
./init.sh example.com
```

Edit `docker-compose.yml`, take note of public ports (80, 9001-9016)

Build and run docker containers

```sh
docker-compose run -d --build
```

That's it.


### Running bots

You can adapt `bot.sh`, it's a wrapper over `lamduct`.
