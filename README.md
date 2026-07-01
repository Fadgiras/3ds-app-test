# 3ds-app-test

Simple application de test pour Nintendo 3DS (homebrew).

## Téléchargement

Les builds sont publiés automatiquement dans les [Releases](../../releases) :

- **`.3dsx`** — à lancer via le *Homebrew Launcher*.
- **`.cia`** — à installer avec *FBI* sur une console moddée.

## Build local

Nécessite [devkitPro](https://devkitpro.org/) (package *3DS Development*).
Pour le `.cia`, il faut aussi `makerom` dans `devkitPro/tools/bin`.

```sh
make        # -> .3dsx
make cia    # -> .cia (utilise banner.bnr, déjà versionné)
```

`banner.bnr` est committé pour que la CI n'ait pas besoin de `bannertool`.
Pour le régénérer après modif de `banner.png` / `banner.wav` :
`make banner` (nécessite `bannertool`).
