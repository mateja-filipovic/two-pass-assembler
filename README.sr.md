<h1 align="center">Dvoprolazni asembler</h1>

<p align="center">
  Dvoprolazni asembler u C++-u za 16-bitni školski procesor: sekcije, simboli, relokacije i devet načina adresiranja.
</p>

<p align="center">
  <a href="README.md">English</a> | <b>Srpski</b>
</p>

<p align="center">
  <a href="#prevođenje">Prevođenje</a> ·
  <a href="#pokretanje">Pokretanje</a> ·
  <a href="#upotreba">Upotreba</a> ·
  <a href="#dizajn">Dizajn</a> ·
  <a href="#testovi">Testovi</a> ·
  <a href="#struktura-projekta">Struktura projekta</a>
</p>

---

Ovo je prvi od tri zadatka iz projekta na predmetu Sistemski softver na Elektrotehničkom fakultetu Univerziteta u Beogradu. Postavka opisuje apstraktni 16-bitni računar i traži asembler, linker i emulator za njega. Ovaj repozitorijum sadrži asembler. On čita izvornu datoteku napisanu na asemblerskom jeziku iz postavke i proizvodi tekstualni predmetni program u školskoj varijanti ELF formata: tabelu sekcija, tabelu simbola, relokacione zapise i mašinski kod, spreman da ga linker poveže sa drugim predmetnim programima. Linker i emulator su zasebni programi i nisu deo ovog repozitorijuma.

## Prevođenje

Potrebni su `g++` i `make`. Projekat se prevodi na Linux-u i macOS-u.

```sh
git clone https://github.com/mateja-filipovic/Two-Pass-Assembler.git
cd Two-Pass-Assembler
make
```

Izvršna datoteka `assembler` nastaje u korenu repozitorijuma. `make clean` uklanja nju i objektne datoteke.

## Pokretanje

```sh
./assembler -o main.o tests/test_main.s
```

Opcija `-o` zadaje ime izlazne datoteke i mora stajati ispred ulazne. Izlaz je običan tekst i može se otvoriti u bilo kom editoru. Pri sintaksnoj grešci asembler ispisuje broj linije i opis problema, ne upisuje ništa i završava sa statusom 1.

## Upotreba

**Format izvornog koda.** Jedna naredba ili direktiva po liniji. Komentar počinje znakom `#` i traje do kraja linije. Labela se završava dvotačkom i mora biti prva na svojoj liniji; može stajati samostalno, i tada označava prvu narednu liniju koja ima sadržaj. Operandi se razdvajaju zapetama. Literali su dekadni ili heksadekadni sa prefiksom `0x` i mogu biti negativni. Registri su `r0` do `r7`, pri čemu je `r6` pokazivač steka, a `r7` programski brojač.

```asm
# file: main.s
.global myStart
.global myCounter
.section myCode
.equ tim_cfg, 0xFF10
myStart:
    ldr r0, $0x1
    str r0, tim_cfg
wait:
    ldr r0, myCounter
    ldr r1, $5
    cmp r0, r1
    jne wait
    halt
.section myData
myCounter:
    .word 0
.end
```

**Direktive.**

| Direktiva | Efekat |
|---|---|
| `.global sym, ...` | Izvozi navedene simbole |
| `.extern sym, ...` | Uvozi navedene simbole |
| `.section ime` | Započinje novu sekciju; prethodna se završava |
| `.word sym, ...` ili `.word literal` | Alocira dva bajta po inicijalizatoru, little-endian |
| `.skip n` | Alocira `n` bajtova popunjenih nulama |
| `.equ sym, literal` | Definiše apsolutni simbol |
| `.end` | Prekida asembliranje; ostatak datoteke se odbacuje |

**Naredbe.** Registarski operandi su označeni sa `regD` i `regS`; `operand` je bilo koji od načina adresiranja navedenih ispod.

| Mnemonik | Efekat |
|---|---|
| `halt` | Zaustavlja procesor |
| `int regD` | Softverski prekid kroz ulaz `regD mod 8` tabele prekida |
| `iret` | Skida `psw` sa steka, zatim `pc` |
| `call operand` | Stavlja `pc` na stek i skače na `operand` |
| `ret` | Skida `pc` sa steka |
| `jmp`, `jeq`, `jne`, `jgt operand` | Skok, bezuslovno ili prema flegovima poslednjeg `cmp` |
| `push regD`, `pop regD` | Rad sa stekom preko `r6`; stek raste ka nižim adresama |
| `xchg regD, regS` | Zamenjuje vrednosti dva registra |
| `add`, `sub`, `mul`, `div`, `cmp regD, regS` | Aritmetika; `cmp` samo postavlja flegove |
| `not regD` | Bitski komplement |
| `and`, `or`, `xor`, `test regD, regS` | Logičke operacije; `test` samo postavlja flegove |
| `shl`, `shr regD, regS` | Pomera `regD` za `regS` bitova |
| `ldr regD, operand` | Učitava `operand` u `regD` |
| `str regD, operand` | Smešta `regD` u `operand` |

**Načini adresiranja.** Naredbe za rad sa podacima i naredbe skoka koriste različitu notaciju za iste hardverske načine adresiranja, jer je kod skoka operand odredišna adresa, a kod `ldr` i `str` sam podatak.

| Operand podatka | Operand skoka | Značenje | Način |
|---|---|---|---|
| `$lit`, `$sym` | `lit`, `sym` | Sama vrednost | neposredno |
| `lit`, `sym` | `*lit`, `*sym` | Reč u memoriji na toj adresi | memorijsko |
| `%sym` | `%sym` | Isto, ali adresirano relativno u odnosu na `pc` | PC relativno |
| `reg` | `*reg` | Registar | registarsko direktno |
| `[reg]` | `*[reg]` | Reč u memoriji na adresi iz `reg` | registarsko indirektno |
| `[reg + lit]`, `[reg + sym]` | `*[reg + lit]`, `*[reg + sym]` | Reč na adresi `reg` plus označeni 16-bitni pomeraj | registarsko indirektno sa pomerajem |

`str` u neposredni operand je greška.

## Dizajn

**Dva prolaza.** Prvi prolaz jednom prolazi kroz datoteku i gradi tabelu simbola: svaka labela, sekcija, `.extern` i `.equ` postaje zapis sa svojom sekcijom i pomerajem, a brojač lokacija se uvećava za veličinu svake naredbe ili direktive. Drugi prolaz ponovo prolazi kroz datoteku, razrešava svaki operand prema gotovoj tabeli, generiše mašinski kod i beleži relokacioni zapis svuda gde će linker morati da upiše vrednost. Oba prolaza klasifikuju operande istim parserom, pa veličina izbrojana u prvom prolazu i kod generisan u drugom ne mogu da se razlikuju.

**Tabela simbola.** Svaki zapis nosi ime, sekciju, pomeraj unutar te sekcije, vidljivost (`l` lokalni ili `g` globalni) i redni broj. Sekcije se pojavljuju kao simboli na pomeraju 0. Uvezeni simboli pripadaju pseudosekciji `UND`, a `.equ` simboli sekciji `ABS`, gde je pomeraj jednak vrednosti literala. Dvaput definisan simbol je greška.

**Kodiranje naredbi.** Naredbe zauzimaju od jednog do pet bajtova: bajt operacionog koda, bajt registara (`regD` u višem nibl-u, `regS` u nižem), bajt načina adresiranja i opcioni dvobajtni sadržaj (engl. payload). Sadržaj se upisuje višim bajtom napred, onako kako ga postavka crta. PC relativni operandi koriste `r7` kao bazni registar, uz registarsko direktno adresiranje sa pomerajem za skokove i registarsko indirektno sa pomerajem za podatke. Reči iz `.word` direktive upisuju se nižim bajtom napred, u skladu sa little-endian memorijom ciljne arhitekture.

**Relokacije.** Relokacioni zapis se beleži za svaki simbol koji nije apsolutan, u listi po sekciji oblika `pomeraj tip simbol`. `pomeraj` je početak naredbe ili `.word` direktive koja sadrži vrednost; kod naredbe se sadržaj nalazi tri bajta dalje. `tip` je `R_HYPO_16` za apsolutne reference i `R_HYPO_PC16` za PC relativne. `simbol` je redni broj u tabeli simbola. Sadržaj već nosi pomeraj simbola unutar njegove sekcije, a linker ga zamenjuje konačnom vrednošću, ne sabira sa njim. Kod PC relativnih referenci asembler prepušta linkeru korekciju za vrednost `pc`.

**Predmetni program.** Tekstualna datoteka sa četiri dela, ovim redom: tabela sekcija, tabela simbola, po jedna lista relokacija za svaku sekciju koja ih ima i mašinski kod grupisan po sekcijama. Predmetni program za gornji primer izgleda ovako.

```
# ------------------ SECTION TABLE ------------------
           NAME           SIZE
         myCode             28
         myData              2

# ------------------ SYMBOL TABLE ------------------
          LABEL        SECTION         OFFSET          SCOPE         NUMBER
         myCode         myCode              0              l              0
        tim_cfg            ABS          65296              l              1
        myStart         myCode              0              g              2
           wait         myCode             10              l              3
         myData         myData              0              l              4
      myCounter         myData              0              g              5

# ------------------ REL.myCode ------------------
10 R_HYPO_16 5
22 R_HYPO_16 3

# ------------------ OBJECT FILE ------------------
# ------------------ CODE.myCode ------------------
A0 00 00 00 01
B0 00 04 FF 10
A0 00 04 00 00
A0 10 00 00 05
74 01
52 F0 00 00 0A
00
# ------------------ CODE.myData ------------------
00 00
```

**Parsiranje.** Parser deli svaku liniju po belinama, odbacuje komentar i zapete i predaje tokene asembleru. Operand u zagradama, kao `[r1 + 5]`, beline mogu da razdvoje na više tokena, pa čitač operanda spaja tokene dok ne naiđe na zatvorenu zagradu. Svaki oblik operanda se poredi sa malim skupom regularnih izraza.

## Testovi

Programi u `tests/` su ulazi kojima je asembler proveravan. Automatski pokretač ne postoji; svaki se asemblira zasebno i izlaz se pregleda.

```sh
for f in tests/*.s; do ./assembler -o "${f%.s}.o" "$f"; done
```

| Datoteka | Proverava |
|---|---|
| `test_one.s` | Većinu načina adresiranja za `ldr`, `str`, skokove i `call`; `.skip`, `.word`, `.equ`, `.extern` i `.global` |
| `test_two.s` | Registarsko indirektno adresiranje sa simbolom i literalom, PC relativni skok, globalne i uvezene simbole |
| `test_main.s`, `test_interrupts.s` | Primer programa iz postavke: tabela prekidnih rutina, rutine za tajmer i terminal u jednoj datoteci, glavna petlja u drugoj |

## Struktura projekta

```
inc/assembler.h    zapisi za simbol, relokaciju, sekciju i operand; klasa Assembler
inc/parser.h       čitač linija i tokenizator
src/assembler.cpp  oba prolaza, parsiranje i kodiranje operanada, upis izlaza
src/parser.cpp     tokenizator
src/main.cpp       obrada komandne linije
tests/             primeri programa
docs/              beleške o implementaciji
Makefile
```

## Ograničenja

- Registri se prihvataju samo kao `r0` do `r7`. Imena `sp`, `pc` i `psw` se ne prepoznaju.
- `.word` prima ili listu simbola ili jedan literal, ne mešovitu listu.
- Ime sekcije sme da se pojavi samo jednom u datoteci; drugi `.section` sa istim imenom se ne spaja sa prvim.
- Opcija `-o` mora stajati ispred ulazne datoteke.
- Predmetni program je isključivo tekstualan. Postavka dozvoljava i dodatni binarni izlaz, koji nije implementiran.
