# 9ime
Plan 9's keyboard driver basically turned ALT into a multi-purpose dead key,
where different sequences of keys following ALT would result in different
characters being emitted. eg.

* _ALT SPACE SPACE_ : `␣`
* _ALT > >_ : `»`
* _ALT a e_ : `æ`
* _ALT ' e_ : `é`

This package brings the feature to windows, though it's only triggered by RIGHT
ALT since ALT is obviously used extensively in windows itself.

# Installation

1. Clone repository
1a. Edit the `keyboard` text file if you want to add to the available sequences
2. `./build.sh`
3. `./9ime`

Note that you won't get any output or feedback from `9ime` to indicate it is running.
To close it open task manager, look for `9ime.exe` in background processes and End Task.

# Overview

The shipped `keyboard` file is essentially the one from [9fans/plan9port](https://github.com/9fans/plan9port) which covers:

* Latin alphabets (eg. Ð Ñ þ æ ę è é ê ë)
* Symbols (eg. £ € © ¡ ¿ °)
* Maths (eg. ÷ ± ² ³ ∀ ∃ ∈ ∑ √ ≠ ≡ ≤)
* Greek (eg. α γ π Α Ω)
* Cyrillic (eg. Π Ε Κ Ο Ρ Α)
* Chess (eg. ♔ ♛ ♝)
* Music (eg. ♩ ♭ ♯)

Plus some additions of my own for Japanese (hiragana and katakana, eg. わたし わ の なまえ わ スクヰーク です).

The sequence for each character is often intuitive -- here's some ground rules:

1. Repeating a character gives you a variant of that character
    * !! becomes ¡
    * || becomes ¦
    * \>\> becomes »
    * == becomes ≡
2. For accents, the accent comes _before_ the character
    * \`A becomes À  (\` = grave)
    * 'A becomes Á  (' = acute)
    * ^A becomes Â  (^ = circumflex)
    * \~A becomes Ã  (\~ = tilde)
    * "A becomes Ä  (" = diæresis)
    * oA becomes Å  (o = ring)
    * ,C becomes Ç  (, = cedilla)
    * ,E becomes Ę  (, = ogonek -- hey these are guidelines)
    * \_e becomes ē  (\_ = macron)
    * ve becomes ě  (v = hacek)
    * ug becomes ğ  (u = breve)
3. Greek characters are prefixed by an asterisk
    * *a becomes α
    * *P becomes Π
    * *R becomes Ρ
4. Cyrillic characters are prefixed by an at sign
    * @a becomes а (which looks indistinguishable from latin in my font...)
    * @Kh becomes Х
    * @@K becomes К (note @@K to avoid conflict with @Kh)
5. Hiragana is prefixed by an h
    * hyo becomes よ
    * he becomes え
    * Hyo becomes ょ (note capital H for small kana)
    * hsi becomes し (note "si" not "shi")
6. Katakana is prefixed by a k
    * kti becomes チ (note "ti" not "chi")
    * Kyu becomes ュ
