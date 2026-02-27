# C++/Java formatter

We use [Clang format](https://clang.llvm.org/docs/ClangFormat.html) to format C++ and Java files.

To format all files that were touched by the last commit:

    git clang-format HEAD^
    git commit -a --amend --no-edit

To format a single file:

    clang-format -i file.cpp file.hpp other_file.cpp

To format the entire repo, run `./tools/unix/clang-format.sh`

# Markdown formatter

We use [Prettier](https://github.com/prettier/prettier) to format Markdown and yaml files.
To format files:
- `npm install prettier`
- `prettier --write README.md docs/*.md tools/python/**/*.md`
- `prettier --write .prettierrc.yaml .forgejo/**/*.yaml .forgejo/*.yml`
