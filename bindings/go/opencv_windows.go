//go:build windows

package opcapi

func (o *Op) CvLoadTemplate(name, filePath string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvLoadTemplate.Call(o.handle, strArg(name), strArg(filePath))
	return int(ret)
}

func (o *Op) CvLoadMaskedTemplate(name, templatePath, maskPath string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvLoadMaskedTemplate.Call(o.handle, strArg(name), strArg(templatePath), strArg(maskPath))
	return int(ret)
}

func (o *Op) CvRemoveTemplate(name string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvRemoveTemplate.Call(o.handle, strArg(name))
	return int(ret)
}

func (o *Op) CvRemoveAllTemplates() int {
	return o.callNoArgs(procCvRemoveAllTemplates)
}

func (o *Op) CvHasTemplate(name string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvHasTemplate.Call(o.handle, strArg(name))
	return int(ret)
}

func (o *Op) CvGetTemplateCount() int {
	return o.callNoArgs(procCvGetTemplateCount)
}

func (o *Op) CvGetAllTemplateNames() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvGetAllTemplateNames.Call(o.handle)
	return wcharString(ret)
}

func (o *Op) CvGetOpenCvVersion() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvGetOpenCvVersion.Call(o.handle)
	return wcharString(ret)
}

func (o *Op) CvLoadTemplateList(templateList string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvLoadTemplateList.Call(o.handle, strArg(templateList))
	return int(ret)
}

func (o *Op) CvToGray(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvToGray, srcFile, dstFile)
}

func (o *Op) CvToBinary(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvToBinary, srcFile, dstFile)
}

func (o *Op) CvToEdge(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvToEdge, srcFile, dstFile)
}

func (o *Op) CvToOutline(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvToOutline, srcFile, dstFile)
}

func (o *Op) CvDenoise(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvDenoise, srcFile, dstFile)
}

func (o *Op) CvEqualize(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvEqualize, srcFile, dstFile)
}

func (o *Op) CvCLAHE(srcFile, dstFile string, clipLimit float64, tileGridSize int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvCLAHE.Call(o.handle, strArg(srcFile), strArg(dstFile), f64Arg(clipLimit), uintptr(tileGridSize))
	return int(ret)
}

func (o *Op) CvBlur(srcFile, dstFile, mode string, kernelSize int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvBlur.Call(o.handle, strArg(srcFile), strArg(dstFile), strArg(mode), uintptr(kernelSize))
	return int(ret)
}

func (o *Op) CvSharpen(srcFile, dstFile string, strength float64) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvSharpen.Call(o.handle, strArg(srcFile), strArg(dstFile), f64Arg(strength))
	return int(ret)
}

func (o *Op) CvCropValid(srcFile, dstFile string) int {
	return o.cvTwoFiles(procCvCropValid, srcFile, dstFile)
}

func (o *Op) CvConnectedComponents(srcFile string, minArea float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvConnectedComponents.Call(o.handle, strArg(srcFile), f64Arg(minArea))
	return wcharString(ret)
}

func (o *Op) CvFindContours(srcFile string, minArea float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvFindContours.Call(o.handle, strArg(srcFile), f64Arg(minArea))
	return wcharString(ret)
}

func (o *Op) CvPreprocessPipeline(srcFile, dstFile, pipeline string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvPreprocessPipeline.Call(o.handle, strArg(srcFile), strArg(dstFile), strArg(pipeline))
	return int(ret)
}

func (o *Op) CvCrop(srcFile string, x, y, width, height int, dstFile string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvCrop.Call(o.handle, strArg(srcFile), uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(dstFile))
	return int(ret)
}

func (o *Op) CvResize(srcFile string, width, height int, dstFile string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvResize.Call(o.handle, strArg(srcFile), uintptr(width), uintptr(height), strArg(dstFile))
	return int(ret)
}

func (o *Op) CvThreshold(srcFile, dstFile string, threshold, maxValue float64, mode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvThreshold.Call(o.handle, strArg(srcFile), strArg(dstFile), f64Arg(threshold), f64Arg(maxValue), strArg(mode))
	return int(ret)
}

func (o *Op) CvInRange(srcFile, dstFile, colorSpace, lower, upper string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvInRange.Call(o.handle, strArg(srcFile), strArg(dstFile), strArg(colorSpace), strArg(lower), strArg(upper))
	return int(ret)
}

func (o *Op) CvMorphology(srcFile, dstFile, mode string, kernelSize, iterations int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvMorphology.Call(o.handle, strArg(srcFile), strArg(dstFile), strArg(mode), uintptr(kernelSize), uintptr(iterations))
	return int(ret)
}

func (o *Op) CvThin(srcFile, dstFile, mode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCvThin.Call(o.handle, strArg(srcFile), strArg(dstFile), strArg(mode))
	return int(ret)
}

func (o *Op) CvMatchTemplate(x, y, width, height int, templateName string, threshold float64, dir, stripMode, method, colorMode int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvMatchTemplate.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateName), f64Arg(threshold), uintptr(dir), uintptr(stripMode), uintptr(method), uintptr(colorMode))
	return wcharString(ret)
}

func (o *Op) CvMatchTemplateScale(x, y, width, height int, templateName, scales string, threshold float64, method, colorMode int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvMatchTemplateScale.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateName), strArg(scales), f64Arg(threshold), uintptr(method), uintptr(colorMode))
	return wcharString(ret)
}

func (o *Op) CvMatchAnyTemplate(x, y, width, height int, templateNames string, threshold float64, dir, stripMode, method, colorMode int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvMatchAnyTemplate.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateNames), f64Arg(threshold), uintptr(dir), uintptr(stripMode), uintptr(method), uintptr(colorMode))
	return wcharString(ret)
}

func (o *Op) CvMatchAllTemplates(x, y, width, height int, templateNames string, threshold float64, dir, stripMode, method, colorMode int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvMatchAllTemplates.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateNames), f64Arg(threshold), uintptr(dir), uintptr(stripMode), uintptr(method), uintptr(colorMode))
	return wcharString(ret)
}

func (o *Op) CvFeatureMatchTemplate(x, y, width, height int, templateName string, threshold float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvFeatureMatchTemplate.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateName), f64Arg(threshold))
	return wcharString(ret)
}

func (o *Op) CvEdgeMatchTemplate(x, y, width, height int, templateName string, threshold float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvEdgeMatchTemplate.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateName), f64Arg(threshold))
	return wcharString(ret)
}

func (o *Op) CvShapeMatchTemplate(x, y, width, height int, templateName string, threshold float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procCvShapeMatchTemplate.Call(o.handle, uintptr(x), uintptr(y), uintptr(width), uintptr(height), strArg(templateName), f64Arg(threshold))
	return wcharString(ret)
}

func (o *Op) cvTwoFiles(proc interface {
	Call(...uintptr) (uintptr, uintptr, error)
}, srcFile, dstFile string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := proc.Call(o.handle, strArg(srcFile), strArg(dstFile))
	return int(ret)
}
