import sys, cv2
img = cv2.imread(sys.argv[1])
for scale in (1, 2, 3):
    big = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_CUBIC)
    text, pts, _ = cv2.QRCodeDetector().detectAndDecode(big)
    if text:
        print(text)
        break
else:
    print("NOT DECODED")
