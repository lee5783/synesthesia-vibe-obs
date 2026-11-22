# Deeplinks & Universal Links

**Analysis Date:** [YYYY-MM-DD]

## Scope & platforms

**Surfaces covered in this document:** [e.g., "iOS app only", "iOS + Android", "Web + native"]

| Platform | Present | Notes |
|----------|---------|-------|
| iOS | [Yes / Not detected] | [Brief: e.g., UIKit/SwiftUI host] |
| Android | [Yes / Not detected] | |
| Web (fallback / PWA) | [Yes / Not detected] | |
| Backend redirects / short links | [Yes / Not detected] | |

---

## Configuration sources (inventory)

List paths only. Do not paste secret values or full hosted JSON contents.

**iOS:**
- URL types / custom schemes: [e.g., `SomeTarget/Info.plist` — CFBundleURLTypes]
- Associated Domains (entitlements): [path to `.entitlements` or note in project]
- Entry hooks: [e.g., `SceneDelegate.swift`, `AppDelegate.swift` — `onOpenURL`, `continue userActivity`]

**Android:**
- Manifest intent-filters: [e.g., `app/src/main/AndroidManifest.xml`]
- App Links / Digital Asset Links: [repo path to `assetlinks.json` if in repo, or "hosted — not in repo"]

**Web / hosting:**
- Redirects or well-known files: [e.g., `public/.well-known/`, `_redirects`, server routes]

**Other:**
- [e.g., React Native `Linking`, Flutter routes — paths]

---

## Universal links & app links

**Verified domains / associated domains:** [List hostnames or "Not detected"]

**Path prefixes / applinks path patterns:** [e.g., `https://example.com/app/*` or "Not detected"]

**Well-known assets (location, not contents):**
- `apple-app-site-association`: [in-repo path or "hosted externally"]
- `assetlinks.json`: [in-repo path or "hosted externally"]

**Notes:** [e.g., staging vs production domains]

---

## Custom URL schemes

| Scheme | Registered in | Parsed by / owner module | Notes |
|--------|---------------|---------------------------|-------|
| `[myapp]` | `[path]` | `[path or module]` | [e.g., dev vs prod] |

If none: **Not detected.**

---

## Routing / handling architecture

**Entry flow:** [Describe the chain from system open-URL callback to UI — one sentence per step with file paths.]

**Router / coordinator / parser:** [Name pattern and primary files: e.g., `DeepLinkRouter.swift`, `NavigationCoordinator`.]

**Prescriptive guidance for new routes:** [Where to register a new path; single place of truth if one exists.]

---

## Route registry (primary mapper)

Fill one row per distinct pattern or feature. Use **Internal** when the URL is only built inside the app; **External** when it can arrive from outside (Safari, email, push, another app). The same logical path may appear twice if both apply.

| Example URL / pattern | Kind (`universal` / `custom_scheme` / `internal_only`) | Internal / External | Resolves to (screen / feature) | Implementation (handler / route id) | Source files |
|----------------------|--------------------------------------------------------|---------------------|--------------------------------|--------------------------------------|--------------|
| `[https://...]` | `[kind]` | `[Internal \| External]` | `[Feature]` | `[handler]` | `` `path` `` |
| `[myapp://...]` | `[kind]` | `[Internal \| External]` | `[Feature]` | `[handler]` | `` `path` `` |

**Optional tags:** If routes are tied to push, widgets, or Siri, add a column or footnote here.

---

## Parameter & query conventions

**Path parameters:** [e.g., `/user/:id` — naming]

**Query keys:** [List known keys or "Not detected"]

**Encoding / reserved characters:** [Notes or "Not applicable"]

---

## Edge cases & fallbacks

**Unknown paths:** [Behavior — e.g., open home, log, ignore]

**Auth / logged-out:** [Behavior]

**Deprecated URLs:** [Aliases or "None documented"]

---

## Testing

**Automated:** [Paths to UI tests / unit tests for deeplinks, or "Not detected"]

**Manual QA:** [How the team verifies links, or "Not documented"]

---

## Gaps

- [Gap 1: e.g., duplicate handlers for same path]
- [Gap 2]

If none known: **None identified.**

---

*Deeplink mapping: [YYYY-MM-DD]*  
*Update when routes, domains, or platform config change.*
