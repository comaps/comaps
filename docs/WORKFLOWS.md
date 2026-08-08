# How works CI?

We use our own server to execute our CI on Codeberg.

- [Android CI](https://codeberg.org/comaps/comaps/src/branch/main/.forgejo/workflows/android-check.yaml)
- [iOS CI](https://github.com/comaps/comaps/actions/workflows/ios-check.yaml)
- [Maps generation CI](https://codeberg.org/comaps/comaps/src/branch/main/.forgejo/workflows/map-generator.yml)

We are not able for now to execute iOS CI on our server, to limit regressions we have enabled Github CI on the [Github Mirror](https://github.com/comaps/comaps) to build the iOS app each time we sync the mirror.
