import AppKit
import Metal

class MetalView : NSView
{
    var _device : MTLDevice!
    var _drawableSize : CGSize!
    
    var _condition : NSCondition!
    var _mutex : pthread_mutex_t!
    var _pthread_cond : pthread_cond_t!
    
    var inflightSemaphores : DispatchSemaphore!         // semaphore for drawable
    
    var _drawable : CAMetalDrawable!        // render context
    
    var _wrapper : Wrapper!
    var _frames : UInt32!
    
    var _bounds : CGSize!

    var metalLayer : CAMetalLayer?
    {
        get
        {
            return self.layer as? CAMetalLayer
        }
        
        set
        {
            layer = newValue
        }
    }
    
    override var wantsLayer: Bool
    {
        get
        {
            return true
        }
        
        set
        {
            wantsLayer = newValue
        }
    }
    
    override init(frame: NSRect)
    {
        super.init(frame: frame)
    }
    
    required init?(coder decoder: NSCoder)
    {
        super.init(coder: decoder)
        
        _condition = NSCondition()
        _mutex = pthread_mutex_t()
        
        var attr: pthread_mutexattr_t = pthread_mutexattr_t()
        pthread_mutexattr_init(&attr)
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)
        let error = pthread_mutex_init(&self._mutex!, &attr)
        pthread_mutexattr_destroy(&attr)
        
        inflightSemaphores = DispatchSemaphore(value: 3)
        
        _drawableSize = CGSize(width: 0.0, height: 0.0)
        
        _wrapper = Wrapper()
        self._device = _wrapper.getDevice()
        
        _bounds = self.bounds.size
        
        self._frames = 0
    }
    
    override func layout() 
    {
        super.layout()
        
        var scale: CGFloat = 1.0
        if(self.window != nil)
        {
            scale = window!.screen!.backingScaleFactor
        }
        
        let drawableSize: CGSize = bounds.size
        
        self.metalLayer?.drawableSize = drawableSize
    }
    
    override func makeBackingLayer() -> CALayer 
    {
        let layer: CAMetalLayer = CAMetalLayer.init()
        layer.wantsExtendedDynamicRangeContent = true
        layer.bounds = self.bounds
        layer.device = self._device
        layer.pixelFormat = MTLPixelFormat.rgb10a2Unorm //  MTLPixelFormat.bgra8Unorm
        layer.displaySyncEnabled = false
        
        let name = CGColorSpace.extendedLinearSRGB
        layer.colorspace = CGColorSpace(name: name)
        
        layer.edrMetadata = CAEDRMetadata.hdr10(minLuminance: 0.0, maxLuminance: 10.0, opticalOutputScale: 1.0)
        layer.framebufferOnly = true
        
        return layer
    }
    
    func beginFrame()
    {
        DispatchQueue.main.sync
        {
            self._drawable = self.metalLayer!.nextDrawable()
            self._wrapper.nextDrawable(
                self._drawable,
                texture: self._drawable.texture,
                width: UInt32(self._bounds.width),
                height: UInt32(self._bounds.height))
        }
    }
    
    func update(time: CGFloat)
    {
        self._wrapper.update(time)
    }
    
    func render()
    {
        self.update(time: 0.0)
        self._wrapper.render()
    }
    
    func endFrame()
    {
        self.inflightSemaphores.signal()
    
        autoreleasepool
        {
            self._drawable = nil
        }
        
        self._frames += 1
    }
}
