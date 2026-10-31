# Changelog

## 0.7.0 - 2026-10-31
- Frei beschreibbarer Titel fuer jede Abfahrtskarte hinzugefuegt.
- Optionaler Bahnsteigfilter hinzugefuegt; die Werte werden wie von der Linz-AG geliefert eingegeben, zum Beispiel `0` oder `1`.
- Die bestehende Mehrfachauswahl fuer Linien bleibt als zusaetzlicher Filter erhalten.

## 0.6.0 - 2026-10-30
- Architektur an das offizielle Tessera-Muster angepasst: ein adaptives Tile statt zweier separater Tile-Typen.
- 1x1 zeigt die naechste Abfahrt gross, breite Karten zeigen automatisch eine Liste.
- Veraltete Live-Daten werden als solche markiert.

## 0.5.2 - 2026-10-30
- Ungueltiges `optional`-Feld aus den Richtungsoptionen entfernt.
- Leere Richtungsfelder bleiben weiterhin automatisch optional.

## 0.5.1 - 2026-10-30
- Richtungsfilter koennen nach der Auswahl wieder abgewaehlt werden.
- Die Tiles erhalten klare Titel und eine staerkere typografische Hierarchie.

## 0.5.0 - 2026-10-30
- Mehrfachauswahl fuer bis zu sechs Linien hinzugefuegt.
- Linienfilterung fuer Mehrfachauswahl robust ins Tile verlagert.
- Exakte Abfahrtszeit neben dem Countdown integriert.

## 0.4.0 - 2026-10-30
- Neues kompaktes „Naechste Abfahrt“-Tile fuer 1x1 und 2x1.
- Zwei unabhaengige Richtungsfilter ergaenzt; beide Richtungen koennen gleichzeitig angezeigt werden.
- Bis zu sechs Linien koennen gleichzeitig ausgewaehlt werden.
- Option zum Ein- und Ausblenden der Bahnsteignummer hinzugefuegt.
- Exakte Abfahrtsuhrzeit kann neben dem Countdown angezeigt werden.
- Countdown- und Linien-Darstellung fuer kleine Karten optimiert.

## 0.3.0 - 2026-10-30
- Abfahrtsliste mit farbigen Linien-Badges, Ziel, Countdown und Bahnsteig neu gestaltet.
- Optionale Filter fuer Linie und Richtung ergaenzt.
- Linien und Richtungen werden aus den Live-Daten der ausgewaehlten Haltestelle geladen.
- Editor-Vorschau an die neue Listenoptik angepasst.

## 0.2.2 - 2026-10-30
- Gespeicherte Haltestellen-Auswahl korrekt aus dem Tessera-Choice-Objekt gelesen.

## 0.2.1 - 2026-10-30
- Haltestellensuche auf die tatsaechliche Linz-AG-Stopfinder-Struktur angepasst.
- Stopfinder auf echte Haltestellen eingeschraenkt, damit die Auswahl eine gueltige Haltestellen-ID liefert.

## 0.2.0 - 2026-10-30
- Eigenes Store-Branding und ausfuehrliche Installationsdokumentation.
- Haltestellensuche, Datenschutz- und Validierungshinweise dokumentiert.

## 0.1.0 - 2026-10-30
- Erste Version mit Live-Abfahrten fuer Linz AG Bus, Obus und Strassenbahn.
- Anzeige von Linie, Ziel, Countdown und Bahnsteig.
