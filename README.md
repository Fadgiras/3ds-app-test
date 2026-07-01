# 3ds-app-test

Simple application de test pour Nintendo 3DS (homebrew).

## Téléchargement

Les builds sont publiés automatiquement dans les [Releases](../../releases) :

- **`.3dsx`** — à lancer via le *Homebrew Launcher*.
- **`.cia`** — à installer avec *FBI* sur une console moddée.

## Build local

Nécessite [devkitPro](https://devkitpro.org/) (package *3DS Development*).
Pour le `.cia`, il faut aussi `makerom` et `bannertool` dans `devkitPro/tools/bin`.

```sh
make        # -> .3dsx
make cia    # -> .cia
```
