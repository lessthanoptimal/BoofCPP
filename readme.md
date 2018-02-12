# Introduction

BoofCPP is a partial port the Java computer vision library [BoofCV](http://boofcv.org) to C++11.
It has been released under an Apache 2.0 open source license and is free to use and redistribute.
Ported operations are typically ones that are time consuming and likely to experience a significant
(at least 2x) speed improvement when compiled as native code. These are typically array heavy operations,
which can be difficult for the Java Virtual Machine (JVM) to fully optimise, e.g. SIMD and remove bounds
checking. Full compatibility (up to floating point precision) with the original library is a design goal
and enforced with unit tests that compare the results generated in BoofCV to BoofCPP.

It's very easy to add BoofCPP to your project. For most people this will involve adding a dependency to their
[Gradle](https://gradle.org/) or [Maven](https://maven.apache.org/) build script and
one line code code to their project. If you wish to build a custom version of BoofCPP
for your architecture please follow the build instructions below. Those who do not RTFM are likely to be punished with
code that is 2x to 3x slower than the pure Java code.

# Adding BoofCPP to Your Project

Pre-build code can be found on Maven Central and added to your project by adding a dependency.
Support is provided for the following architectures android-arm, linux-x86_64, macosx-x86_64, and windows-x86_64.
Examples for how to add depedencies in a Gradle script are provided below.

```Gradle
compile group: 'org.boofcpp', name: "boofcpp", version: '0.29-SNAPSHOT'
```

That just includes the Java library. Now you need to tell it which architecture to include
by specifying an architecture in the classifier field, e.g.

```Gradle
compile group: 'org.boofcpp', name: "boofcpp", version: '0.29-SNAPSHOT', classifier: "linux-x86_64"
```

C++ code is injected into BoofCV through its BOverride API. Before invoking certain functions or creating
certain classes BoofCV will check to see if there exists a BOverride. If so it will invoke that instead.

```Java
BoofCPP.initialize()
```

Now all functions which BoofCPP supports will be overrided. This will also override any previously loaded overrides.
Initialize must be called before any image processing is done.

# Building

CMake is used to build the source code and BoofCPP should compile without issue on most
platforms with a C++11 compiler. There are no external dependencies. To get
the fastest code make sure you build it in release mode! By default it will
build in debug mode. Debug mode is great for development but many times slower
than release code.

```bash
cd boofcpp
mkdir build;cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```

Then to install the Java library invoke the following command:
```bash
cd boofcpp/jni
./gradlew install
```

# Design Comments

BoofCPP has been designed and built to feel like a C++ library and not a Java library, but has Java roots.
In some cases limitations from Java are inherited, e.g. limits on array sizes and focus on certain data types.
Function and class names have been tweaked to follow C++ standards.

Changes from BoofCV
* Use of unsigned data types.
  * Justification: F*** Java's lack of unsigned data types and the hacks it forces upon people
* Array length's are unsigned 32-bit integerss (uint32_t)
  * Justification: Simplifies precondition checks, e.g. impossible to pass in a negative number
* Templates
  * Justification: Despite all of it's flaws, C++ templates are easier to maintain than autogenerating code.
* Bloated Unit Tests:
  * Justification: Java reflections make iterating through functions and data types much easier and more concise

# Performance Comments

Java is much faster than most non-Java developers believe. Saying if C++
or Java is faster is a complex question. My general observations is
that: "It varies. For most well written code, C++ is equal to or faster
than Java. However, most code isn't well written". For array heavy tasks
C++ tends to be 1.5x to 2.5x faster, if the array is large enough.
Non-array tasks it's a coin toss which one is faster, but in general they
are comparable. Situations have been encounter in BoofCPP where
equivalent Java code runs much faster than C++ code and it's not fully
understood why.

The answer also depends on which platform you're using and compiler
flags for C++. Oracle's JVM does a good to excellent job of optimizing
code at runtime. Google's Dalvik VM does an OK job and produces code
which is relatively
much slower than the JVM. Google's official stance appears to be that
they no longer want to bother with making Dalvik faster and just tell
people to write native code if performance is an issue.

It's worth mentioning that the reason Java was used as the language for
BoofCV is because Java projects are easier to write and maintain as
they get larger, plus the development tools are better.
