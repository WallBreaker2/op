//go:build windows

package opcapi

func (o *Op) AStarFindPath(mapWidth, mapHeight int, disablePoints string, beginX, beginY, endX, endY int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procAStarFindPath.Call(
		o.handle,
		uintptr(mapWidth),
		uintptr(mapHeight),
		strArg(disablePoints),
		uintptr(beginX),
		uintptr(beginY),
		uintptr(endX),
		uintptr(endY),
	)
	return wcharString(ret)
}

func (o *Op) FindNearestPos(allPos string, typ, x, y int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindNearestPos.Call(o.handle, strArg(allPos), uintptr(typ), uintptr(x), uintptr(y))
	return wcharString(ret)
}
