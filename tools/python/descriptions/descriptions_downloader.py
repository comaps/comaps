import json
import logging
import os
import random
import time
import types
import urllib.error
import urllib.parse
import http.client

from concurrent.futures import ThreadPoolExecutor

import htmlmin
import requests
import wikipediaapi
from bs4 import BeautifulSoup
from wikidata.client import Client

from descriptions.exceptions import GettingError, ParseError

"""
This script downloads Wikipedia pages for different languages.
"""
log = logging.getLogger(__name__)

WORKERS = 80
REQUEST_ATTEMPTS = 8
ATTEMPTS_PAUSE_SECONDS = 4.0

HEADERS = {f"h{x}" for x in range(1, 7)}
BAD_SECTIONS = {
    "en": [
        "External links",
        "Sources",
        "See also",
        "Bibliography",
        "Further reading",
        "References",
    ],
    "de": [
        "Einzelnachweise",
        "Weblinks",
        "Literatur",
        "Siehe auch",
        "Anmerkungen",
        "Anmerkungen und Einzelnachweise",
        "Filme",
        "Einzelbelege",
    ],
    "fr": [
        "Bibliographie",
        "Lien externe",
        "Voir aussi",
        "Liens externes",
        "Références",
        "Notes et références",
        "Articles connexes",
    ],
    "es": ["Vínculos de interés", "Véase también", "Enlaces externos", "Referencias"],
    "ru": ["Литература", "Ссылки", "См. также", "Библиография", "Примечания"],
    "tr": ["Kaynakça", "Ayrıca bakınız", "Dış bağlantılar", "Notlar", "Dipnot"],
}


def try_get(obj, prop, *args, **kwargs):
    attempts = REQUEST_ATTEMPTS
    while attempts != 0:
        try:
            attr = getattr(obj, prop)
            is_method = isinstance(attr, types.MethodType)
            return attr(*args, **kwargs) if is_method else attr
        except (
            requests.exceptions.ConnectionError,
            requests.exceptions.ReadTimeout,
            json.decoder.JSONDecodeError,
            http.client.HTTPException,
        ) as e:
            log.debug(e)
        except urllib.error.HTTPError as e:
            if e.code == 404:
                raise GettingError(f"Page not found {e.msg}")
        except KeyError:
            raise GettingError(f"Getting {prop} field failed. {prop} not found.")
        except urllib.error.URLError:
            raise GettingError(f"URLError: {obj}, {prop}, {args}, {kwargs}")

        time.sleep(random.uniform(0.0, ATTEMPTS_PAUSE_SECONDS))
        attempts -= 1

    raise GettingError(f"Getting {prop} field failed")


def read_popularity(path):
    """
    :param path: a path of popularity file. A file contains '<id>,<rank>' rows.
    :return: a set of popularity object ids
    """
    ids = set()
    for line in open(path):
        try:
            ident = int(line.split(",", maxsplit=1)[0])
        except (AttributeError, IndexError):
            continue
        ids.add(ident)
    return ids


def should_download_page(popularity_set):
    def wrapped(ident):
        return popularity_set is None or ident in popularity_set

    return wrapped


def remove_bad_sections(soup, lang):
    if lang not in BAD_SECTIONS:
        return soup
    it = iter(soup.find_all())
    current = next(it, None)
    current_header_level = None
    while current is not None:
        if current.name in HEADERS and current.text.strip() in BAD_SECTIONS[lang]:
            current_header_level = current.name
            current.extract()
            current = next(it, None)
            while current is not None:
                if current.name == current_header_level:
                    break
                current.extract()
                current = next(it, None)
        else:
            current = next(it, None)
    return soup


def beautify_page(html, lang):
    soup = BeautifulSoup(html, "html.parser")
    for x in soup.find_all():
        if len(x.text.strip()) == 0:
            x.extract()
    soup = remove_bad_sections(soup, lang)
    html = str(soup.prettify())
    html = htmlmin.minify(html, remove_empty_space=True)
    return html


def need_lang(lang, langs):
    return lang in langs if langs else True


def get_page_info(url):
    url = urllib.parse.unquote(url)
    parsed = urllib.parse.urlparse(url)
    try:
        lang = parsed.netloc.split(".", maxsplit=1)[0]
    except (AttributeError, IndexError):
        raise ParseError(f"{parsed.netloc} is incorrect.")
    try:
        page_name = parsed.path.rsplit("/", maxsplit=1)[-1]
    except (AttributeError, IndexError):
        raise ParseError(f"{parsed.path} is incorrect.")
    return lang, page_name


def get_wiki_page(lang, page_name):
    wiki = wikipediaapi.Wikipedia(
        language=lang, extract_format=wikipediaapi.ExtractFormat.HTML
    )
    return wiki.page(page_name)


def download(directory, url):
    try:
        lang, page_name = get_page_info(url)
    except ParseError:
        log.exception(f"Parsing failed. {url} is incorrect.")
        return None

    path = os.path.join(directory, f"{lang}.html")
    if os.path.exists(path):
        log.debug(f"{path} already exists.")
        return None

    page = get_wiki_page(lang, page_name)
    try:
        text = try_get(page, "text")
    except GettingError as e:
        log.exception(f"Error: page {page_name} is not downloaded for lang {lang} and url {url} ({e}).")
        return None

    page_size = len(text)
    if page_size > 0:
        os.makedirs(directory, exist_ok=True)
        text = beautify_page(text, lang)
        log.info(f"Save to {path} {lang} {page_name} {page_size}.")
        with open(path, "w") as file:
            file.write(text)
    else:
        log.warning(f"Page {url} is empty. It has not been saved.")

    return text


def get_wiki_langs(url):
    lang, page_name = get_page_info(url)
    page = get_wiki_page(lang, page_name)

    curr_lang = [(lang, url)]
    try:
        langlinks = try_get(page, "langlinks")
        return (
            list(zip(langlinks.keys(), [link.fullurl for link in langlinks.values()]))
            + curr_lang
        )
    except GettingError as e:
        log.exception(f"Error: no languages for page {page_name} with url {url} ({e}).")
        return curr_lang


def download_all_from_wikipedia(path, url, langs):
    try:
        available_langs = get_wiki_langs(url)
    except ParseError:
        log.exception("Parsing failed. {url} is incorrect.")
        return
    available_langs = filter(lambda x: need_lang(x[0], langs), available_langs)
    for lang in available_langs:
        download(path, lang[1])


def wikipedia_worker(output_dir, checker, langs):
    def wrapped(line):
        if not line.strip():
            return
        try:
            # First param is mwm_path, which added this line entry.
            _, ident, url = line.split("\t")
            ident = int(ident)
            if not checker(ident):
                return
            url = url.strip()
        except (AttributeError, ValueError):
            log.exception(f"{line} is incorrect.")
            return
        parsed = urllib.parse.urlparse(url)
        path = os.path.join(output_dir, parsed.netloc, parsed.path[1:])
        download_all_from_wikipedia(path, url, langs)

    return wrapped


def download_from_wikipedia_tags(input_file, output_dir, langs, checker):
    with open(input_file) as file:
        _ = file.readline() # skip header
        with ThreadPoolExecutor(WORKERS) as pool:
            pool.map(wikipedia_worker(output_dir, checker, langs), file)


def get_wikidata_urls(entity, langs):
    try:
        keys = entity.data["sitelinks"].keys()
    except (KeyError, AttributeError):
        log.exception(f"Sitelinks not found for {entity.id}.")
        return None
    return [
        entity.data["sitelinks"][k]["url"]
        for k in keys
        if any([k.startswith(lang) for lang in langs])
    ]


def wikidata_worker(output_dir, checker, langs):
    def wrapped(line):
        if not line.strip():
            return
        try:
            ident, wikidata_id = line.split("\t")
            ident = int(ident)
            wikidata_id = wikidata_id.strip()
            if not checker(ident):
                return
        except (AttributeError, ValueError):
            log.exception(f"{line} is incorrect.")
            return
        client = Client()
        try:
            entity = try_get(client, "get", wikidata_id, load=True)
        except GettingError:
            log.exception(f"Error: page is not downloaded {wikidata_id}.")
            return
        urls = get_wikidata_urls(entity, langs)
        if not urls:
            return
        path = os.path.join(output_dir, wikidata_id)
        for url in urls:
            download(path, url)

    return wrapped


def download_from_wikidata_tags(input_file, output_dir, langs, checker):
    wikidata_output_dir = os.path.join(output_dir, "wikidata")
    os.makedirs(wikidata_output_dir, exist_ok=True)
    with open(input_file) as file:
        with ThreadPoolExecutor(WORKERS) as pool:
            pool.map(wikidata_worker(wikidata_output_dir, checker, langs), file)


def check_and_get_checker(popularity_file):
    popularity_set = None
    if popularity_file is None:
        log.warning(f"Popularity file not set.")
    elif os.path.exists(popularity_file):
        popularity_set = read_popularity(popularity_file)
        log.info(f"Popularity set size: {len(popularity_set)}.")
    else:
        log.error(f"Popularity file ({popularity_file}) not found.")
    return should_download_page(popularity_set)
