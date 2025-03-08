
import AppKit


class ViewController: NSViewController {
    
    var _renderView : MetalView!
    var _displayLink : CADisplayLink!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        _renderView = self.view as? MetalView
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @objc func renderLoop(_ sender: AnyObject?)
    {
        self._renderView!.beginFrame()
        self._renderView!.render()
        self._renderView!.endFrame()
    }
    
    override func viewDidAppear()
    {
        let selector = #selector(renderLoop(_:))
        self._displayLink = _renderView.displayLink(target: self, selector: selector)
        self._displayLink?.add(to: .main, forMode: .common)
    }

}

