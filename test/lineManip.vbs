Do While Not WScript.StdIn.AtEndOfStream
    WScript.StdOut.WriteLine("Edited" & WScript.StdIn.ReadLine())
Loop
