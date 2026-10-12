# Finding Prime Number Till Computer Crashes

> *"Your computer was not designed for this. Let's find out exactly how not designed."*

A tiny C++ program that does exactly what it says on the tin:
it finds prime numbers **until your computer crashes**.

It uses every CPU core you've got, and it **keeps every single prime it finds in RAM**,
forever, with no plan to let go. Eventually the memory runs out, the program notices,
writes its last words to a file, and dies with dignity.

---

## ✨ Features

- 🧮 **Segmented Sieve of Eratosthenes**: a 2,000-year-old algorithm, now with multithreading
- 🔥 **Uses all CPU cores**: your fans will finally get some exercise
- 🧠 **Hoards every prime in memory**: about 200 MB of RAM per second, give or take
- 📊 **Live stats**: primes found, biggest prime, speed, RAM usage
- 😰 **Computer mood meter**: from `chillin'` to `writing its will` to `seeing the light`
- 💾 **`progress.txt`**: saved every 10 seconds, so a record survives even if your PC doesn't

## 🖥️ Sample output

```
[00:00:06] primes: 105,878,213 | biggest: 2,164,260,851 | 24,418,131/s | RAM: 1,299 MB | system 76% (sweating)
```

## 🚀 How to run

**Requirements:** Windows, Visual Studio 2026 (Community is fine) with the
*Desktop development with C++* workload.

1. Open `FindingPrimeNumberTillComputerCrashes.slnx`
2. Switch the configuration to **Release | x64** (Debug is for cowards and is also 20x slower)
3. Press **Ctrl+F5**
4. Watch your RAM go up
5. Regret

Or from a Developer PowerShell:

```powershell
msbuild FindingPrimeNumberTillComputerCrashes.slnx /p:Configuration=Release /p:Platform=x64
.\bin\Release\FindingPrimeNumberTillComputerCrashes.exe
```

## ⚠️ Disclaimer

> [!WARNING]
> This program **intentionally** eats all your memory. Once RAM is full, Windows starts
> swapping to disk and everything becomes *very* slow. Save your work, close your
> 47 Chrome tabs, and do not run this on anything you love.

The author takes no responsibility for crashed computers, lost homework,
or existential crises caused by staring at prime numbers.

## 🏆 Hall of Fame

Did your machine die a heroic death? Open a PR and add your record:

| Machine | RAM | Survived | Biggest prime | Last words |
|---|---|---|---|---|
| *your PC here* | ? GB | ??:??:?? | ? | *"it's fine, everything is fine"* |

## 📜 License

MIT. Do whatever you want with it. Your computer, your funeral.
