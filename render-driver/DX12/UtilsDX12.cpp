#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/ImageDX12.h>
#include <Windows.h>


#include <LogPrint.h>
#include <wtfassert.h>
#include <d3d12.h>

namespace RenderDriver
{
    namespace DX12
    {
		namespace Utils
		{
			/*
			**
			*/
			std::wstring convertToLPTSTR(std::string const& str)
			{
				int32_t iNumChars = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), (int32_t)str.length(), NULL, 0);
				std::wstring wstrTo;
				if(iNumChars)
				{
					wstrTo.resize(iNumChars);
					if(MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), (int32_t)str.length(), &wstrTo[0], iNumChars))
					{
						return wstrTo;
					}
				}

				return std::wstring();
			}

			/*
			**
			*/
			void transitionBarrier(
				RenderDriver::Common::CBuffer& buffer,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				RenderDriver::Common::ResourceStateFlagBits before,
				RenderDriver::Common::ResourceStateFlagBits after)
			{
				//WTFASSERT(buffer.getState() == before, "Wrong state, saved: %d given: %d", before, buffer.getState());

				RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);
				RenderDriver::DX12::CBuffer& dx12Buffer = static_cast<RenderDriver::DX12::CBuffer&>(buffer);

				D3D12_RESOURCE_STATES dx12ResourceBefore = SerializeUtils::DX12::convert(before);
				D3D12_RESOURCE_STATES dx12ResourceAfter = SerializeUtils::DX12::convert(after);

				D3D12_RESOURCE_BARRIER transitionBarrier;
				memset(&transitionBarrier, 0, sizeof(transitionBarrier));
				transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				if(commandBuffer.getType() == RenderDriver::Common::CommandBufferType::Copy)
				{
					if(after == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess)
					{
						transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
					}
					else
					{
						if(before == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess)
						{
							transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
						}
					}
				}

				transitionBarrier.Transition.pResource = static_cast<ID3D12Resource*>(dx12Buffer.getNativeBuffer());
				transitionBarrier.Transition.StateBefore = dx12ResourceBefore;
				transitionBarrier.Transition.StateAfter = dx12ResourceAfter;
				transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList())->ResourceBarrier(1, &transitionBarrier);

				buffer.setState(after);
			}

			/*
			**
			*/
			void transitionBarrier(
				RenderDriver::Common::CImage& image,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				RenderDriver::Common::ResourceStateFlagBits before,
				RenderDriver::Common::ResourceStateFlagBits after)
			{
				RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);
				RenderDriver::DX12::CImage& dx12Image = static_cast<RenderDriver::DX12::CImage&>(image);

				D3D12_RESOURCE_STATES dx12ResourceBefore = SerializeUtils::DX12::convert(before);
				D3D12_RESOURCE_STATES dx12ResourceAfter = SerializeUtils::DX12::convert(after);

				D3D12_RESOURCE_BARRIER transitionBarrier;
				memset(&transitionBarrier, 0, sizeof(transitionBarrier));
				transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				transitionBarrier.Transition.pResource = static_cast<ID3D12Resource*>(dx12Image.getNativeImage());
				transitionBarrier.Transition.StateBefore = dx12ResourceBefore;
				transitionBarrier.Transition.StateAfter = dx12ResourceAfter;
				transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList())->ResourceBarrier(1, &transitionBarrier);
			}

			/*
			**
			*/
			uint32_t alignForUavCounter(uint32_t iBufferSize)
			{
				uint32_t const iAlignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
				return (iBufferSize + (iAlignment - 1)) & ~(iAlignment - 1);
			}

			/*
			**
			*/
			void copyBuffer(
				RenderDriver::Common::CBuffer& srcBuffer,
				RenderDriver::Common::CBuffer& destBuffer,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				uint64_t iSrcBufferOffset,
				uint64_t iDestBufferOffset,
				uint64_t iSize)
			{
				RenderDriver::DX12::CBuffer& srcBufferDX12 = static_cast<RenderDriver::DX12::CBuffer&>(srcBuffer);
				RenderDriver::DX12::CBuffer& destBufferDX12 = static_cast<RenderDriver::DX12::CBuffer&>(destBuffer);
				RenderDriver::DX12::CCommandBuffer& commandBufferDX12 = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

				RenderDriver::Common::ResourceStateFlagBits prevDestState = destBuffer.getState();
				RenderDriver::Common::ResourceStateFlagBits prevSrcState = srcBuffer.getState();

				if(prevSrcState != RenderDriver::Common::ResourceStateFlagBits::CopySource)
				{
					RenderDriver::DX12::Utils::transitionBarrier(
						srcBuffer,
						commandBuffer,
						prevSrcState,
						RenderDriver::Common::ResourceStateFlagBits::CopySource);
				}

				if(prevDestState != RenderDriver::Common::ResourceStateFlagBits::CopyDestination)
				{
					RenderDriver::DX12::Utils::transitionBarrier(
						destBuffer,
						commandBuffer,
						prevDestState,
						RenderDriver::Common::ResourceStateFlagBits::CopyDestination);
				}
				
				static_cast<ID3D12GraphicsCommandList*>(commandBufferDX12.getNativeCommandList())->CopyBufferRegion(
					static_cast<ID3D12Resource*>(destBufferDX12.getNativeBuffer()),
					iDestBufferOffset,
					static_cast<ID3D12Resource*>(srcBufferDX12.getNativeBuffer()),
					iSrcBufferOffset,
					iSize);

				if(prevSrcState != RenderDriver::Common::ResourceStateFlagBits::CopySource)
				{
					RenderDriver::DX12::Utils::transitionBarrier(
						srcBuffer,
						commandBuffer,
						RenderDriver::Common::ResourceStateFlagBits::CopySource,
						prevSrcState);
				}

				if(prevDestState != RenderDriver::Common::ResourceStateFlagBits::CopyDestination)
				{
					RenderDriver::DX12::Utils::transitionBarrier(
						destBuffer,
						commandBuffer,
						RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
						prevDestState);
				}
			}

			/*
			**
			*/
			void copyBufferToTexture(
				RenderDriver::Common::CBuffer& srcBuffer,
				RenderDriver::Common::CImage& destImage,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				uint64_t iSrcBufferOffset,
				uint32_t iWidth,
				uint32_t iHeight,
				uint32_t iNumChannels,
				uint32_t iChannelSize)
			{
				RenderDriver::DX12::CBuffer& bufferDX12 = static_cast<RenderDriver::DX12::CBuffer&>(srcBuffer);
				RenderDriver::DX12::CImage& imageDX12 = static_cast<RenderDriver::DX12::CImage&>(destImage);
				RenderDriver::DX12::CCommandBuffer& commandBufferDX12 = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

				RenderDriver::DX12::Utils::transitionBarrier(
					bufferDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess,
					RenderDriver::Common::ResourceStateFlagBits::CopySource);

				RenderDriver::DX12::Utils::transitionBarrier(
					imageDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::Common,
					RenderDriver::Common::ResourceStateFlagBits::CopyDestination);

				auto format = SerializeUtils::DX12::convert(destImage.getFormat());

				D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
				{
					static_cast<ID3D12Resource*>(bufferDX12.getNativeBuffer()),             //ID3D12Resource* pResource;
					D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,      // D3D12_TEXTURE_COPY_TYPE Type;
					{
						iSrcBufferOffset,                                                      // UINT64 Offset
						{
							format,              // DXGI_FORMAT Format;
							iWidth,                                        // UINT Width;
							iHeight,                                       // UINT Height;
							1,                                                  // UINT Depth;
							iWidth * iNumChannels * iChannelSize     // UINT RowPitch;
						}
					},
				};

				D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
				{
					static_cast<ID3D12Resource*>(imageDX12.getNativeImage()),             //ID3D12Resource* pResource;
					D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,      // D3D12_TEXTURE_COPY_TYPE Type;
					0
				};

				static_cast<ID3D12GraphicsCommandList*>(commandBufferDX12.getNativeCommandList())->CopyTextureRegion(
					&destCopyLocation,
					0,
					0,
					0,
					&srcCopyLocation,
					nullptr);

				RenderDriver::DX12::Utils::transitionBarrier(
					bufferDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::CopySource,
					RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess);

				RenderDriver::DX12::Utils::transitionBarrier(
					imageDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
					RenderDriver::Common::ResourceStateFlagBits::Common);
			}

			/*
			**
			*/
			void copyTextureToBuffer(
				RenderDriver::Common::CImage& srcImage,
				RenderDriver::Common::CBuffer& destBuffer,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				uint64_t iDestBufferOffset,
				uint32_t iWidth,
				uint32_t iHeight,
				uint32_t iNumChannels,
				uint32_t iChannelSize)
			{
				RenderDriver::DX12::CBuffer& bufferDX12 = static_cast<RenderDriver::DX12::CBuffer&>(destBuffer);
				RenderDriver::DX12::CImage& imageDX12 = static_cast<RenderDriver::DX12::CImage&>(srcImage);
				RenderDriver::DX12::CCommandBuffer& commandBufferDX12 = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

				RenderDriver::DX12::Utils::transitionBarrier(
					destBuffer,
					commandBuffer,
					destBuffer.getState(),
					RenderDriver::Common::ResourceStateFlagBits::CopyDestination);

				RenderDriver::DX12::Utils::transitionBarrier(
					srcImage,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::Common,
					RenderDriver::Common::ResourceStateFlagBits::CopySource);

				D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
				{
					static_cast<ID3D12Resource*>(bufferDX12.getNativeBuffer()),							//ID3D12Resource* pResource;
					D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,											// D3D12_TEXTURE_COPY_TYPE Type;
					{
						iDestBufferOffset,																// UINT64 Offset
						{
							SerializeUtils::DX12::convert(RenderDriver::Common::Format::R32G32B32A32_FLOAT),     // DXGI_FORMAT Format;
							iWidth,																		// UINT Width;
							iHeight,																	// UINT Height;
							1,																			// UINT Depth;
							iWidth * iNumChannels * iChannelSize										// UINT RowPitch;
						}
					},
				};

				D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
				{
					static_cast<ID3D12Resource*>(imageDX12.getNativeImage()),							//ID3D12Resource* pResource;
					D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,											// D3D12_TEXTURE_COPY_TYPE Type;
					0
				};

				static_cast<ID3D12GraphicsCommandList*>(commandBufferDX12.getNativeCommandList())->CopyTextureRegion(
					&destCopyLocation,
					0,
					0,
					0,
					&srcCopyLocation,
					nullptr);

				RenderDriver::DX12::Utils::transitionBarrier(
					bufferDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::CopyDestination,
					RenderDriver::Common::ResourceStateFlagBits::Common);

				RenderDriver::DX12::Utils::transitionBarrier(
					imageDX12,
					commandBuffer,
					RenderDriver::Common::ResourceStateFlagBits::CopySource,
					RenderDriver::Common::ResourceStateFlagBits::Common);
			}

			/*
			**
			*/
			void showDebugBreadCrumbs(RenderDriver::DX12::CDevice& device)
			{
				ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(device.getNativeDevice());
				HRESULT hr = nativeDevice->GetDeviceRemovedReason();

				ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
				if(!SUCCEEDED(nativeDevice->QueryInterface(IID_PPV_ARGS(&pDred))))
				{
					assert(0);
				}
				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput;
				D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;

				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput1;
				pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput1);
				if(!SUCCEEDED(pDred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput)))
				{
					return;
				}
				if(!SUCCEEDED(pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput)))
				{
					return;
				}

				D3D12_AUTO_BREADCRUMB_NODE1 const* pCurr = DredAutoBreadcrumbsOutput1.pHeadAutoBreadcrumbNode;
				for(uint32_t i = 0; i < 1024; i++)
				{
					if(pCurr == nullptr)
					{
						assert(0);
						break;
					}

					char acCommandListName[1024];
					convertWChar(acCommandListName, pCurr->pCommandListDebugNameW, 1024);

					char acCommandQueueName[1024];
					convertWChar(acCommandQueueName, pCurr->pCommandQueueDebugNameW, 1024);

					DEBUG_PRINTF("%d command list: %s command queue: %s\n",
						i,
						acCommandListName,
						acCommandQueueName);


					if(pCurr->pBreadcrumbContexts)
					{
						for(uint32_t j = 0; j < pCurr->BreadcrumbContextsCount; j++)
						{
							char acContext[1024] = { 0 };
							uint32_t iBreadCrumbIndex = 0;

							convertWChar(acContext, pCurr->pBreadcrumbContexts[j].pContextString, 1024);
							iBreadCrumbIndex = pCurr->pBreadcrumbContexts[j].BreadcrumbIndex;
							auto op = pCurr->pCommandHistory[iBreadCrumbIndex];

							DEBUG_PRINTF("context %d: %s\n", iBreadCrumbIndex, acContext);
						}
					}

					for(uint32_t j = 0; j < pCurr->BreadcrumbCount; j++)
					{
						auto type = pCurr->pCommandHistory[j];
						std::string enumStr = "";
						switch(type)
						{
						case 0: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_SETMARKER"; break; }
						case 1: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT"; break; }
						case 2: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_ENDEVENT"; break; }
						case 3: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED"; break; }
						case 4: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED"; break; }
						case 5: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT"; break; }
						case 6: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DISPATCH"; break; }
						case 7: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION"; break; }
						case 8: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION"; break; }
						case 9: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE"; break; }
						case 10: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_COPYTILES"; break; }
						case 11: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE"; break; }
						case 12: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW"; break; }
						case 13: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW"; break; }
						case 14: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW"; break; }
						case 15: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER"; break; }
						case 16: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE"; break; }
						case 17: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_PRESENT"; break; }
						case 18: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA"; break; }
						case 19: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION"; break; }
						case 20: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION"; break; }
						case 21: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME"; break; }
						case 22: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES"; break; }
						case 23: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT"; break; }
						case 24: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64"; break; }
						case 25: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION"; break; }
						case 26: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE"; break; }
						case 27: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1"; break; }
						case 28: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION"; break; }
						case 29: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2"; break; }
						case 30: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1"; break; }
						case 31: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE"; break; }
						case 32: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO"; break; }
						case 33: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE"; break; }
						case 34: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS"; break; }
						case 35: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND"; break; }
						case 36: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND"; break; }
						case 37: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION"; break; }
						case 38: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP"; break; }
						case 39: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1"; break; }
						case 40: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND"; break; }
						case 41: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND"; break; }
						case 42: { enumStr = "D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH"; break; }
						}

						DEBUG_PRINTF("\t%d %s\n",
							j,
							enumStr.c_str());
					}

					pCurr = pCurr->pNext;
					if(pCurr == nullptr)
					{
						break;
					}
				}
				
			}

			/*
			**
			*/
			void transitionBarriers(
				RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
				RenderDriver::Common::CCommandBuffer& commandBuffer,
				uint32_t iNumBarrierInfo,
				bool bReverseState)
			{
				RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);
				D3D12_RESOURCE_BARRIER aDX12Barriers[64];
				uint32_t iNumValidBarriers = 0;
				for(uint32_t iBarrierInfo = 0; iBarrierInfo < iNumBarrierInfo; iBarrierInfo++)
				{
					RenderDriver::Common::Utils::TransitionBarrierInfo const& barrierInfo = aBarrierInfo[iBarrierInfo];

					RenderDriver::Common::ResourceStateFlagBits beforeState = (bReverseState == false) ? barrierInfo.mBefore : barrierInfo.mAfter;
					RenderDriver::Common::ResourceStateFlagBits afterState = (bReverseState == false) ? barrierInfo.mAfter : barrierInfo.mBefore;

					D3D12_RESOURCE_STATES dx12ResourceBefore = SerializeUtils::DX12::convert(beforeState);
					D3D12_RESOURCE_STATES dx12ResourceAfter = SerializeUtils::DX12::convert(afterState);

					D3D12_RESOURCE_BARRIER transitionBarrier;
					memset(&transitionBarrier, 0, sizeof(transitionBarrier));
					transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					if(barrierInfo.mbBeginOnly)
					{
						transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
					}
					else if(barrierInfo.mbEndOnly)
					{
						transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
					}

					if(commandBuffer.getType() == RenderDriver::Common::CommandBufferType::Copy)
					{
						if(beforeState == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess)
						{
							transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
						}
						else if(afterState == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess)
						{
							//transitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
							transitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
						}
					}

					transitionBarrier.Transition.pResource = nullptr;
					if(barrierInfo.mpBuffer)
					{
						transitionBarrier.Transition.pResource = static_cast<ID3D12Resource*>(static_cast<RenderDriver::DX12::CBuffer*>(barrierInfo.mpBuffer)->getNativeBuffer());
					}
					else if(barrierInfo.mpImage)
					{
						transitionBarrier.Transition.pResource = static_cast<ID3D12Resource*>(static_cast<RenderDriver::DX12::CImage*>(barrierInfo.mpImage)->getNativeImage());
					}

					if(transitionBarrier.Transition.pResource)
					{
						transitionBarrier.Transition.StateBefore = dx12ResourceBefore;
						transitionBarrier.Transition.StateAfter = dx12ResourceAfter;
						transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

						aDX12Barriers[iNumValidBarriers] = transitionBarrier;
						++iNumValidBarriers;
					}

					if(aBarrierInfo[iBarrierInfo].mpBuffer)
					{
						aBarrierInfo[iBarrierInfo].mpBuffer->setState(afterState);
					}
				}

				if(iNumValidBarriers > 0)
				{
					static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList())->ResourceBarrier(iNumValidBarriers, aDX12Barriers);
				}
			}

		}	// Utils

    }	// DX12

}	// RenderDriver