package app.organicmaps.sdk.util.concurrency;

import androidx.core.util.Function;

import java.util.concurrent.Executor;

public class AsyncResult<T> {

  public interface SuccessHandler<T> {
    void handle(T value);
  }

  public interface FailureHandler {
    void handle(Throwable error);
  }

  private final Executor executor;

  private SuccessHandler<T> successHandler;
  private FailureHandler failureHandler;

  private boolean completed = false;
  private T value;
  private Throwable error;

  public AsyncResult(Executor executor) {
    this.executor = executor;
  }

  public void complete(T value) {
    this.completed = true;
    this.value = value;
    dispatch();
  }

  public void fail(Throwable error) {
    this.completed = true;
    this.error = error;
    dispatch();
  }

  private void dispatch() {
    executor.execute(() -> {
      if (error != null && failureHandler != null) {
        failureHandler.handle(error);
      } else if (value != null && successHandler != null) {
        successHandler.handle(value);
      }
    });
  }

  public AsyncResult<T> onSuccess(SuccessHandler<T> handler) {
    this.successHandler = handler;
    if (completed && error == null) dispatch();
    return this;
  }

  public AsyncResult<T> onFailure(FailureHandler handler) {
    this.failureHandler = handler;
    if (completed && error != null) dispatch();
    return this;
  }

  public <R> AsyncResult<R> thenApply(Function<T, R> mapper) {
    AsyncResult<R> next = new AsyncResult<>(executor);

    onSuccess(value -> {
      try {
        next.complete(mapper.apply(value));
      } catch (Throwable t) {
        next.fail(t);
      }
    });

    onFailure(next::fail);
    return next;
  }

  public <R> AsyncResult<R> thenCompose(Function<T, AsyncResult<R>> mapper) {
    AsyncResult<R> next = new AsyncResult<>(executor);

    onSuccess(value -> {
      try {
        mapper.apply(value)
            .onSuccess(next::complete)
            .onFailure(next::fail);
      } catch (Throwable t) {
        next.fail(t);
      }
    });

    onFailure(next::fail);
    return next;
  }
}

