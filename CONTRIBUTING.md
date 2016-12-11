## Contributing

Pull requests are welcome.

### Supported Qt Version
CuteCom Version 0.3x was the first version based on Qt5. Originally it was developed using Qt 5.3.
Reported in issue #8 there are still users relying on Qt5 in a version less then 5.3.
Travis is currently setup to test the code against Qt version 5.4
(https://travis-ci.org/cyc1ingsir/cutecom).

Please verify, that your CRs don't rely on a higher Qt version. 

### Consistent Formatting

CuteCom is being developed with a code styleguide in place since I 
believe that a consisten coding style is important.
To achive that, clang-format is beeing used.
For a great talk on that you may want to watch this video
https://www.youtube.com/watch?v=s7JmdCfI__c

Before you create a pull request, please make sure the code adheres 
to the style.

```
BasedOnStyle: llvm, ColumnLimit: 120, IndentWidth: 4, Standard: Cpp11,
PointerBindsToType: false, BreakBeforeBraces: Linux,
BreakConstructorInitializersBeforeComma: true, AccessModifierOffset: -4,
BreakBeforeBinaryOperators: true
```

You may do so by using the Qt Creator beautifier.

(To get the menu entry displayed, the beautifier plugin needs to be enablde under Help -> About Plugins.)

### Qt Creator Beautifier

![](clang_format_01.png)
![](clang_format_02.png)

For __Qt Creator version 4.x__ and above the syntax is different. Each setting needs to be entered on a new line: 
```
BasedOnStyle: llvm 
ColumnLimit: 120
IndentWidth: 4
Standard: Cpp11,
PointerBindsToType: false
BreakBeforeBraces: Linux
BreakConstructorInitializersBeforeComma: true
AccessModifierOffset: -4
BreakBeforeBinaryOperators: true
```

### clang-format on CLI
This is based on clang-format version 3.5.0:
>clang-format --version
clang-format version 3.5.0 (tags/RELEASE_350/final 216961)

> clang-format  -i -style="{BasedOnStyle: llvm, ColumnLimit: 120, IndentWidth: 4, Standard: Cpp11, PointerBindsToType: false, BreakBeforeBraces: Linux, BreakConstructorInitializersBeforeComma: true, AccessModifierOffset: -4, BreakBeforeBinaryOperators: true}" mainwindow.cpp

