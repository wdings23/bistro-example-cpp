
import AppKit


class ViewController: NSViewController {
    
    var _renderView : MetalView!
    var _displayLink : CVDisplayLink!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        _renderView = self.view as? MetalView
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    override func viewDidAppear() {
        _renderView = self.view as? MetalView
        
        CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink)
        CVDisplayLinkSetOutputCallback(
            _displayLink!,
            {
                (_, _, _, _, _, self) in
                
                // this view controller
                let viewController : ViewController = unsafeBitCast(self, to: ViewController.self)
                
                // begin render frame
                viewController._renderView!.beginFrame()
                
                viewController._renderView!.render()
                
                viewController._renderView!.endFrame()
                
                return kCVReturnSuccess
            },
            UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque()))
        
        CVDisplayLinkStart(_displayLink!)
    }


}

