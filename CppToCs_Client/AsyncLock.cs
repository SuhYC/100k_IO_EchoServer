using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace CsMemcpyIOTest
{
    internal class AsyncLock
    {
        private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);

        /// <summary>
        /// 사용법 :
        /// private static readonly AsyncLock _asyncLock = new AsyncLock(); <br/>
        /// using (await _asyncLock.LockAsync()) { ... }
        /// </summary>
        /// <returns></returns>
        public async Task<IDisposable> LockAsync()
        {
            await _semaphore.WaitAsync();
            return new Handler(_semaphore);
        }

        private sealed class Handler : IDisposable
        {
            private readonly SemaphoreSlim _semaphore;
            private bool _disposed = false;

            public Handler(SemaphoreSlim semaphore)
            {
                _semaphore = semaphore;
            }


            /// <summary>
            /// IDisposable 인터페이스를 상속하여
            /// using 스코프의 실행이 끝날 때 자동으로 호출된다.
            /// 스코프를 벗어날 때가 아닌 끝날 때 호출되므로
            /// 원하는 시점까지 lock이 유지된다.
            /// </summary>
            public void Dispose()
            {
                if (!_disposed)
                {
                    _semaphore.Release();
                    _disposed = true;
                }
            }
        }
    }
}
