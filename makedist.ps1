$out="JakED"
rm $out -r -fo
md $out
cp jaked.exe -Destination $out
cp LICENSE -Destination $out
cp doc\Commands.md -Destination $out
cp doc\EdCheatSheet.md -Destination $out
cp doc\Global.md -Destination $out
cp doc\Ranges.md -Destination $out
cp doc\Regexp.md -Destination $out
$compress = @{
Path="$($out)"
CompressionLevel = "Optimal"
DestinationPath = "$($out)\JakED-$($args[0]).zip"
}
Compress-Archive @compress
