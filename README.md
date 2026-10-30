# Linz Linien Abfahrten fuer Tessera

Ein Tessera-Plugin fuer Live-Abfahrten der LINZ AG Linien. Es unterstuetzt Bus, Obus und Strassenbahn und zeigt pro Abfahrt Linie, Ziel, Countdown und - sofern vorhanden - den Bahnsteig.

## Einrichtung

1. Das Plugin in Tessera ueber einen Ordner oder GitHub-Link hinzufuegen.
2. Das Tile auf dem Panel platzieren.
3. Im Tile-Inspector nach der gewuenschten Haltestelle suchen und sie aus der Liste auswaehlen.

Eine Haltestellen-ID muss nicht bekannt sein. Tessera sucht die Haltestelle direkt ueber den offiziellen Linz-AG-Endpunkt und verwendet die interne ID automatisch.

Die Daten werden hoechstens einmal pro Minute ueber `www.linzag.at` aktualisiert. Die API-Antwort wird im Home-Assistant-Teil von Tessera abgerufen und auf die wenigen Felder reduziert, die das Panel benoetigt.

## Datenschutz

Fuer die Abfrage werden der eingegebene Haltestellenname und die Anfrage an den oeffentlichen Linz-AG-Endpunkt uebertragen. Es werden keine Zugangsdaten benoetigt.
