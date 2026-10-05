# Components

Everything needed for **one board**. The full BOM generated from the KiCad project, with
manufacturer part numbers, is in [`production/planch-bom.csv`](../production/planch-bom.csv).

{% set shops = {"digikey": "DigiKey", "aliexpress": "AliExpress", "amazon": "Amazon"} %}
{% macro link(l) -%}
{%- if l.shop == "amazon" -%}
[{{ shops[l.shop] }}](https://www.amazon.it/dp/{{ l.id }}{% if config.extra.amazon_tag %}?tag={{ config.extra.amazon_tag }}{% endif %}){: rel="sponsored nofollow" }
{%- elif l.shop == "aliexpress" -%}
[{{ shops[l.shop] }}](https://www.aliexpress.com/item/{{ l.id }}.html)
{%- else -%}
[{{ shops[l.shop] }}]({{ l.url }})
{%- endif %} {{ l.label }}
{%- endmacro %}

| Ref. | Part | Qty | Side | Package | Where to buy |
| --- | --- | :-: | --- | --- | --- |
{% for c in components -%}
| {{ c.ref }} | **{{ c.name }}**{% if c.mpn %}<br>`{{ c.mpn }}`{% endif %}{% if c.detail %}<br><small>{{ c.detail }}</small>{% endif %} | {{ c.qty }} | {{ c.side }} | {{ c.package }} | {% for l in c.links %}{{ link(l) }}{% if not loop.last %}<br>{% endif %}{% endfor %}{% if not c.links %}any, see notes{% endif %} |
{% endfor %}

## Notes

- **ESP32-S3-Zero**: buy the version **without** pre-soldered pin headers. The board is
  soldered by its castellated edges.
- **OLED**: most 0.96" I²C modules look alike, but the pin order is not standard. PlancH needs
  **GND, VIN, SCK, SDA** from left to right with the pins on top.
- **5050 LEDs**: buy the **RGB** version (3 channels), not RGBW. Mixing an RGBW LED into the
  chain shifts the colours of every LED after it.
- **Switches**: generic 6 × 6 × 4.3 mm SMD switches often have a slightly different pad
  layout from the C&K PTS645. Check the first one before soldering all ten.
- **Quantities**: SMD parts come in tapes of 10–100. Buying for several boards (or friends)
  costs almost the same as buying for one.

{% if config.extra.amazon_tag %}
!!! info "Affiliate links"
    Amazon links are affiliate links: as an Amazon Associate I earn from qualifying purchases,
    at no extra cost to you. It helps keep the project going.
{% endif %}

## Desk stand

The optional [desk stand](../accessories/desk-stand.md) needs 1 × PLA print (about 35 g) and
4 × rubber bumpers Ø 10 mm.
