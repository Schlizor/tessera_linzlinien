# Validation

## Lokale Checks

- `tessera-plugin.yaml` parses as YAML.
- `translations/*.json` parse as JSON.
- `components/linz_departures/__init__.py` passes Python compilation.
- The manifest uses plugin API `0.12`, matching the searchable-choice feature.
- The stop finder map returns `ref.id` values and filters to transport stops.
- The departures map reads `stopEvents[*]` and maps live/planned times as epochs.

## Live API smoke test

Use the stop finder with `Hauptbahnhof` and pass the returned `ref.id` to `XML_DM_REQUEST`. A successful response contains `stopEvents`, `transportation.number`, `transportation.destination.name` and `departureTimeEstimated`.

## Tessera preview

For the complete simulator check, use a clean checkout of the current Tessera plugin repository and run its `tools/preview.py` setup followed by `tools/check.py` and the strict preview. A real ESPHome build and hardware test remain the final acceptance step.
