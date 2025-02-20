import AppKit
import Metal

class MetalView : NSView
{
    var _device : MTLDevice!
    var _drawableSize : CGSize!
    
    var _condition : NSCondition!
    var _mutex : pthread_mutex_t!
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
        
        var drawableSize: CGSize = bounds.size
        //drawableSize.width *= scale
        //drawableSize.height *= scale
        
        self.metalLayer?.drawableSize = drawableSize
    }
    
    override func makeBackingLayer() -> CALayer 
    {
        let layer: CAMetalLayer = CAMetalLayer.init()
        layer.bounds = self.bounds
        layer.device = self._device
        layer.pixelFormat = MTLPixelFormat.bgra8Unorm
        layer.displaySyncEnabled = true
        
        return layer
    }
    
    func beginFrame()
    {
        DispatchQueue.main.async
        {
            self._drawableSize = self.window!.frame.size
            
            
            pthread_mutex_lock(&self._mutex!)
            // get the next drawable
            self._drawable = nil
            pthread_mutex_unlock(&self._mutex!)
            
            while(self._drawable == nil)
            {
                pthread_mutex_lock(&self._mutex!)
                self._drawable = self.metalLayer!.nextDrawable()
                pthread_mutex_unlock(&self._mutex!)
                
                self._wrapper.nextDrawable(
                    self._drawable,
                    texture: self._drawable.texture,
                    width: UInt32(self._bounds.width),
                    height: UInt32(self._bounds.height))
            }
            
        }
        
        autoreleasepool
        {
            // wait until we have a valid drawable
            self.inflightSemaphores.wait()
            while(self._drawable == nil)
            {
                usleep(1)
            }
            
            // inform wrapper of the new drawable
            pthread_mutex_lock(&self._mutex!)
            self._wrapper.nextDrawable(
                self._drawable,
                texture: self._drawable.texture,
                width: UInt32(self._bounds.width),
                height: UInt32(self._bounds.height))
            pthread_mutex_unlock(&self._mutex!)
        }
    }
    
    func update(time: CGFloat)
    {
        self._bounds = self.bounds.size
        self._wrapper.update(time)
    }
    
    func render()
    {
        self._wrapper.render()
    }
    
    func endFrame()
    {
        self.inflightSemaphores.signal()
        
        pthread_mutex_lock(&self._mutex!)
        
        autoreleasepool
        {
            self._drawable = nil
        }
        
        pthread_mutex_unlock(&self._mutex!)
        
        self._frames += 1
    }
}
