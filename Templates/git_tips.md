[header]: # (To generate a html version of this document:)
[pandoc]: # (pandoc git_tips.md -c github.css -o git_tips.html -s --self-contained)

# Git Tips

## Setup

```
git config --global user.name "John Doe"
git config --global user.email johndoe@example.com
git config --global push.default simple
```

---

## SSH-Key

```
ssh-keygen -t rsa -b 4096 -C "johndoe@example.com"
```

---

## Useful Configurations

### Case Sensitivity

```
git config --global core.ignorecase false
```

### Color:

```
git config --global color.ui "true"
```

### Global `.gitignore`: 

```
git config --global core.excludesfile "~/.gitignore"
```

---

## Useful Aliases

```
git config --global alias.unstage "reset HEAD --"
git config --global alias.st "status -s"
git config --global alias.lol "log --oneline --graph --decorate"
git config --global alias.last "log -1 HEAD"
git config --global alias.co "checkout"
git config --global alias.ci "commit"
git config --global alias.br "branch"
```

---
