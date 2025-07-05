# Cherteau

Requires [Egg](https://github.com/aksommerville/egg) to build.

Code For A Cause Micro Jam, July 2025, theme "EVERY ACTION HAS A COST".

- Formal RPG, single-screen outer world maps.
- Combat will be mini-games a la Bellacopia Maleficia, but maybe only 3 or 4 of them.
- Every screen has toll doors, whose tolls increase with every step you take.
- Ideal play has you limiting activity in new rooms, until you've farmed enough gold to open all the doors.

## Agenda

- Thu 3 July: Navigable outer world, placeholder combat, toll doors. -OK
- Fri 4 July: Mini games. -2/4
- Sat 5 July: Full world, graphics.
- Sun 6 July: Hello, game over, audio, polish.
- Submissions open until noon Monday. Submit by EOD Sunday.

## TODO

- [x] Placeholder graphics.
- [x] Map loader.
- [x] Session state. ...yeah yeah yeah, just dump everything in (g).
- [x] Layers.
- [x] Sprites.
- [x] Toll doors.
- [x] Trigger encounters.
- [x] Transitions.

- [x] Karate chop contest.
- [x] Dance off.
- [x] Axe catching contest. ...nix it. I can't make it look good, and having second thoughts about the violence.
- [x] Jump rope contest.

- [x] Treasure chests.
- [ ] Fairies: Present like minigames, but you can't lose. Free coins or heart, all you have to do is catch it.
- [x] Safety carpet
- [ ] Treadle doors.

- [x] Maps.
- [ ] Final graphics.
- [ ] Music.
- [ ] Sound effects.
- [ ] Hello.
- [ ] Game over.
- [ ] Victory.

- [ ] "Visited" and "seen" maps in minimap.
- [ ] Animate spending gold.
- [ ] Visual juice paying tolldoor.
- [ ] Animate walking.
- [ ] Animate bumping walls.
- [ ] Karate: Foe should get the same thing as you, and should use exactly enough power to break it.
- [ ] Karate: Fudge the mavg such that the first stroke creates visible activity in the meter.
- [ ] Karate: Facial expressions.
- [ ] Karate: Indicator in power meter of required level. Or does that expose too much?
- [ ] Dance: At the end, I keep wanting to press A and proceed, before the "You win" message.
- [ ] Jumprope: Distraught faces when lost. And some decoration on the rope. An explosion?
- [x] Dance off animates choppy native. Check `egg_audio_get_playhead`, are we estimating driver buffer position incorrectly?
- - ...Pulse has its own buffer, and we asked it to set at 50 hz, while we thought it was 100 hz. Fixed, and it's a little less choppy now. Still not great.
- [x] Is there a flash of random content when launching dance off? Or my eyes playing tricks on me...
- - ...yes. We need to ensure it updates before the first render.

- [ ] Itch page.
