# Reaching the DroidDeck developers / Как обратиться к разработчикам

Checked / Проверено: **2026-10-26**. These are public project channels, not a
promise of a reply. No message has been sent on the owner's behalf.

## Recommended route / Рекомендуемый путь

1. **GitHub Show and tell:**
   https://github.com/Droid-Deck/DroidDeck/discussions/categories/show-and-tell
   Publish one focused showcase with the fork, test APK, Mavk and short gameplay
   video. This is a better fit than a generic bug report for a working fork.
   Опубликуй один технический showcase со ссылками на форк, APK, Mavk и видео.
   Это подходящий раздел для демонстрации работающей интеграции.
2. **Official Discord / Официальный Discord:** https://discord.gg/JRGAvawjsm
   The upstream README links this server. Share a short introduction plus the
   discussion link; ask staff for the appropriate development channel if unclear.
   Название отдельного dev-канала не проверено: не предполагаем, что он есть.
   В upstream README подтверждён форум **#bug-reports**, но он предназначен для
   ошибок, а не для дублирования рекламного обращения.
3. **Relevant public contributors / Участники разработки:**
   - [xXJSONDeruloXx](https://github.com/xXJSONDeruloXx): a leading contributor and
     author of several recent upstream commits when checked.
   - [The412Banner](https://github.com/The412Banner): a leading contributor in the
     public contribution list.
   - [maxjivi05](https://github.com/maxjivi05): a substantial contributor, also
     credited by upstream for runtime/input/controller work.
   These are code-contribution observations, not verified staff titles or private
   contact details. If useful, mention one relevant contributor in the discussion;
   avoid tagging everyone or posting the same request repeatedly.

## Make review easy / Как облегчить ревью

* Lead with a **60–120 second gameplay clip**, device/GPU, resolution and settings.
  Отдельно укажи, что HL2 30+ FPS и местами 60 — наблюдение в игре, не locked 60.
* Provide the **direct test APK link** and the short UI-only setup guide. The
  reviewer should not need the seed archive, a compiler or terminal commands.
* Ask a narrow question first: review AHB allocation/import, fences, compositor
  layer eligibility and resize/input. These pieces can be useful independently
  of adopting the experimental Mavk shader compatibility layer.
* Clearly list remaining artifact/performance/compatibility limitations. Preserve
  upstream licenses and distinguish this fork from an official release.
* The unsent editable draft is [UPSTREAM_MESSAGE.md](UPSTREAM_MESSAGE.md).
  Gameplay video is pending the owner's Google Drive link.

## Primary sources / Первичные источники

* Upstream README/community: https://github.com/Droid-Deck/DroidDeck#community
* Discussion category: https://github.com/Droid-Deck/DroidDeck/discussions/categories/show-and-tell
* Public contributor list: https://api.github.com/repos/Droid-Deck/DroidDeck/contributors?per_page=5
* Recent public authors: https://api.github.com/repos/Droid-Deck/DroidDeck/commits?per_page=5

Личные email/DM-контакты не угадывались. Ни один канал не гарантирует ответа;
короткое воспроизведение, APK и выделенный участок кода повышают удобство ревью.
