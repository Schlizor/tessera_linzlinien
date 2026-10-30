# Linz Linien Abfahrten für Tessera

Ein Tessera-Plugin für Live-Abfahrten der Linz AG Linien. Es unterstützt Bus, Obus und Straßenbahn und zeigt pro Abfahrt Linie, Ziel, Countdown und – sofern vorhanden – den Bahnsteig.

## Einrichtung

1. Das Plugin in Tessera über einen Ordner oder GitHub-Link hinzufügen.
2. Im Tile-Inspector die Linz-AG-Haltestellen-ID eintragen.
3. Das Tile auf dem Panel platzieren.

Die Haltestellen-ID kann über die Linz-AG-Suche ermittelt werden. Für `Hauptbahnhof` ist sie beispielsweise `60501720`.

Die Daten werden höchstens einmal pro Minute über `www.linzag.at` aktualisiert. Die API-Antwort wird im Home-Assistant-Teil von Tessera abgerufen und auf die wenigen Felder reduziert, die das Panel benötigt.

## Datenschutz

Für die Abfrage werden die Haltestellen-ID und die Anfrage an den öffentlichen Linz-AG-Endpunkt übertragen. Es werden keine Zugangsdaten benötigt.
