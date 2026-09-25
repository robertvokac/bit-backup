# Bit Backup website

This directory contains the English presentation site for Bit Backup. It is a static website with no build step or external dependencies.

Open `index.html` directly in a browser, or serve this directory locally:

```bash
python3 -m http.server 8000 --directory web
```

Then visit `http://localhost:8000`. The content is based on the repository's README and current CLI implementation. Update the website when CLI behavior changes.

## Deployment

The site is published at [bitbackup.robertvokac.com](https://bitbackup.robertvokac.com/) by the GitHub Pages workflow in `.github/workflows/deploy-pages.yml`. A push to `develop` deploys the `web/` directory only when that push changes a file under `web/`. Changes outside `web/` do not trigger a deployment.

The custom domain is configured in the repository's GitHub Pages settings, and its DNS CNAME points to `robertvokac.github.io`.
