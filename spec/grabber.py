#!/usr/bin/env python3
import json
import os
import requests

from urllib.parse import urljoin


# Map Viewer: http://punter.inf.ed.ac.uk/graph-viewer/?map=/maps/sample.json


def download(url, target, file_name):
    file_name = os.path.join(target, file_name)
    with open(file_name, 'wb') as fd:
        print(url)
        r = requests.get(url, stream=True)
        for chunk in r.iter_content():
            if chunk:
                fd.write(chunk)
    return file_name


def grab_maps(target):
    data_url = 'http://punter.inf.ed.ac.uk/graph-viewer/maps.json'

    os.makedirs(target, exist_ok=True)

    fn = download(data_url, target, 'maps.json')
    with open(fn, 'r') as fd:
        maps = json.load(fd)

    def get_from_list(items):
        for obj in items:
            url = urljoin(data_url, obj['filename'])
            fn = url.rsplit('/', 1)[-1]
            download(url, target, fn)

    get_from_list(maps['maps'])
    get_from_list(maps['other_maps'])


if __name__ == '__main__':
    grab_maps('maps')
