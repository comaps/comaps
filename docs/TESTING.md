# Testing

User testing is an important part of the pre-release process to make sure no bugs make it into the final release version.

You can install test builds, which are pre-release versions of our app, and try to find new bugs before they make it to more people. You should be aware though that those test builds can be more unstable and prone to errors than the released versions. So don't use those test builds, if you really need to rely on our app working as stable and bug-free as possible.

The main focus of testing those test builds should always be the newly introduced or changed parts.
[Finding and reporting other already existing bugs](https://codeberg.org/comaps/comaps/issues) of course is important too, but it is not the principal purpose of the test builds.

This is the process for the different platforms:

## Android

- A new Issue is created for every Android test build.
- The Issue includes the APK, that needs to be tested, and notes of what changed with this test build.
- Comments and problems found with the specific test build should be noted in the Issue.
- The existing test build Issue will be closed with a link to the new test build Issue, if a new test build becomes available.
- New test builds also usually get announced in the chats.

## iOS

- A new Issue is created for every iOS test build.
- The Issue includes the signup link for the TestFlight and notes of what changed with this test build.
- The available space for beta testers in TestFlight is limited and all spots might be filled already. We regularly remove inactive testers and sometimes add more spots though. So if the beta is full, maybe check again at some point later.
- Comments and problems found with the specific test build should be noted in the Issue.
- The existing test build Issue will be closed with a link to the new test build Issue, if a new test build becomes available.
- TestFlight normally notifies existing testers of a new test build and new test builds also usually get announced in the chats.
