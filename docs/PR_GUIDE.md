# Pull Request Guidelines

This document gives some guidelines to write and review PR with essential elements.

## Writing a Pull Request (PR):

- A PR should have a reasonable size and reflect only one idea, a feature, a bugfix, or a refactoring. This helps to review and limit regressions in the app
- If you want to work on more than one feature/bug/refactoring at once, please create separate PRs for these.
- New functionality and unit or integration tests for it should be developed in the same PR
- Every commit of all PRs should be compilable under all platforms and all tests should pass. If changes break unit or integration tests, then these tests should be fixed, ideally before opening a PR. 
- Every commit should reflect a completed idea and have an understandable comment. Review fixes should be merged into one commit
- When some source files are changed and then some other source files based on them are auto-generated, they should be committed in different commits. For example, if you change style (mapcss) files, then put auto-generated files into a separate [styles] Regenerate commit
- Whitespace and formatting changes should be a separate commit from logical changes, and possibly even be a separate cleanup PR, to make reviewing and merging easier.
- All commits and PR captions should be written in English.
- We suggest PRs should have prefixes in square brackets depending on the changed subsystem. For example, [routing], [generator], or [android]. Commits may have several prefixes (See `git log --oneline|egrep -o '\[[0-9a-z]*\]'|sort|uniq -c|sort -nr|less` for ideas.)
- Use imperative mood in commit's message, e.g. `[core] Fix gcc warnings` not `[core] Fixed gcc warnings`
- All code bases should conform to ./docs/CPP_STYLE.md, ./docs/OBJC_STYLE.md, ./docs/JAVA_STYLE.md or other style in ./docs/ depending on the language
- The description field of every PR should contain a description to explain **what and why** vs. how.
- If your changes are visual (e.g. app UI or map style changes) then please add before/after screenshots or videos.
- Link Codeberg issues into the description field, [See tutorial](https://forgejo.org/docs/latest/user/linked-references/)

## Review a Pull Request (PR):

- We expect all comments to be polite, including for PR reviews. For PRs feedback should also only concern the PR
- It's better to ask the developer to make the code simpler or add comments to the codebase than to understand the code through the developer's explanation
- If a developer changes the PR significantly, the reviewers who have already approved the PR should be informed about these changes
- A reviewer should pay attention not only to the code base but also to the description of the PR and commits
- We prefer PRs to be approved by at least two reviewers. To have a different vision about how the feature/bugs are implemented or fixed, to help to find bugs and test the PR
- If a reviewer doesn't have time to review all the PR they should write about it explicitly. For example, LGTM for android part
- If a reviewer and a developer cannot find a compromise, a third opinion should be sought
- A PR which should not be merged after review should be marked as a draft.

### Reviewing pull requests of first-time contributors

First-time contributors are likely to be less familiar with both the project code but also the way CoMaps as a community operates.
Some of them might also not be too familiar with using git (e.g. for rebasing branches or signing DCOs).

For those reasons they might require more effort from reviewers. 
At the same time, new contributors are the lifeblood that keeps CoMaps sustainable by ensuring that enough people are contributing code. For that reason, it's important to help those newcomers in their on-boarding.

To help newcomers in feeling welcome and getting onboarded, some suggestions for what to include:

- Thank the contributor for opening a PR (they volunteered time to help the project! And offering code to a new community can be nerve-wrecking.)
- Where applicable:
    * Notes on how to sign the DCO/rebase commits
    * The necessary constructive feedback if there are things that need to be improved before a PR can be merged
- Give the contributor another thank you for their effort, signal that the community is open for helping further (e.g. on Codeberg and link to Matrix/TG/Zulip)

Here's a small template that can help writing feedback to first-time contributors:

> Thank you so much for this contribution - and welcome to CoMaps!
> 
> For CoMaps, we use the [_Developers Certficate of Origin_](https://codeberg.org/comaps/Governance/src/branch/main/DCO.md) for contributors to certify that they wrote/have the right to submit their code to our project. This helps ensure that CoMaps can remain open source as it gives us some legal protection. You can sign it by signing your commit. The easiest way is to amend your existing commit with `git commit -s -m 'your commit message'`. If you are using the Codeberg interface you can also sign your commit by selection _"Add a Signed-off-by trailer by the committer at the end of the commit log message."_. 
> 
> If you haven't yet, you could also add yourself to our [CONTRIBUTOR](https://codeberg.org/comaps/comaps/src/branch/main/CONTRIBUTORS) file with your name or pseudonym if you want (that would also give you a chance to sign the commit if needed).

> There are a few things that could help improve your PR/would need to be fixed / I have a few questions about… (the actual PR feedback)
> 
> I hope that helps! If anything is unclear, please don't hesitate to get in touch on here, or reach out via our chat channels. You can find most of us [on different Matrix/Telegram channels](https://codeberg.org/comaps/Governance/wiki/Chat-rooms) or [on our contributor Zulip](https://comaps.zulipchat.com/join/e5e3c4zurmfxykrtbiiq6qrw/).
> 
> Thanks again for putting this PR together!
   

## Recommendations for making PRs:

- Functions and methods should not be long. In most cases, it's good if the whole body of a function or method fits on the monitor. It's good to write a function or a method shorter than 60 lines
- If you are solving a big task it's worth splitting it into subtasks and developing one or several PRs for every subtask.
- In most cases refactoring should be done in a separate PR
- If you want to refactor a significant part of the codebase, it's worth discussing it with all developers in an issue before starting work
- It's worth using the 'Resolve' conversation button to minimize the list of comments in a PR
