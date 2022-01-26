@echo off

echo "Testing BC1 decompression -> kodim23_bc1.dds"
bcdec.exe "./test_images/kodim23_bc1.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC2 decompression -> testcard_bc2.dds"
bcdec.exe "./test_images/testcard_bc2.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC3 decompression -> dice_bc3.dds"
bcdec.exe "./test_images/dice_bc3.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC4 decompression -> dice_bc4.dds"
bcdec.exe "./test_images/dice_bc4.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC5 decompression -> dice_bc5.dds"
bcdec.exe "./test_images/dice_bc5.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC6H decompression -> lythwood_room_1k_bc6h_signed.dds"
bcdec.exe "./test_images/lythwood_room_1k_bc6h_signed.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")

echo "Testing BC7 decompression -> dice_bc7.dds"
bcdec.exe "./test_images/dice_bc7.dds"
if %errorlevel% neq 0 (echo "Decompression failed :(") else (echo "Succeeded!")
